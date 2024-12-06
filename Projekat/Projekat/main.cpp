#define _CRT_SECURE_NO_WARNINGS

#define CRES 30

#include <glad.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <cstdlib>  // For rand()
#include <ctime>    // For seeding rand()
//#include <GL/glew.h>   
#include <GLFW/glfw3.h>

//
#include <glm/glm.hpp>            // Core features
#include <glm/gtc/matrix_transform.hpp> // Transformations
#include <glm/gtc/type_ptr.hpp>  

//text
#include <ft2build.h>
#include FT_FREETYPE_H


#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include <map>
#include <Shader.cpp>

using namespace std;
unsigned int compileShader(GLenum type, const char* source); 
unsigned int createShader(const char* vsSource, const char* fsSource);
static unsigned loadImageToTexture(const char* filePath); //Ucitavanje teksture, izdvojeno u funkciju
void renderStars(int count, GLuint starShader, GLuint VAO, int wWidth, int wHeight);
void RenderTextLetterByLetter(Shader& shader, std::string text, float x, float y, float scale, glm::vec3 color, float delay, GLFWwindow* window);
void RenderTextLetterByLetterWithFade(Shader& shader, std::string text, float x, float y, float scale, glm::vec3 color, float delay = 0.2f, float fadeDuration = 2.0f);

struct Character {
    unsigned int TextureID; // ID handle of the glyph texture
    glm::ivec2   Size;      // Size of glyph
    glm::ivec2   Bearing;   // Offset from baseline to left/top of glyph
    unsigned int Advance;   // Horizontal offset to advance to next glyph
};
std::map<GLchar, Character> Characters;

unsigned int textVAO, textVBO;

int main(void)
{

    // ++++++++++++++++++++++++++++++++++++++++++++++++++++++ INICIJALIZACIJA ++++++++++++++++++++++++++++++++++++++++++++++++++++++

    if (!glfwInit()) 
    {
        std::cout<<"GLFW Biblioteka se nije ucitala! :(\n";
        return 1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);


    GLFWwindow* window;
    unsigned int wWidth = 1920;
    unsigned int wHeight = 1080;
    const char wTitle[] = "Desert";
    window = glfwCreateWindow(wWidth, wHeight, wTitle, glfwGetPrimaryMonitor(), NULL); 

    if (window == NULL) 
    {
        std::cout << "Prozor nije napravljen! :(\n";
        glfwTerminate(); 
        return 2; 
    }
   
    glfwMakeContextCurrent(window);
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

   

    Shader shader("text.vert", "text.frag");

    glm::mat4 projection = glm::ortho(0.0f, static_cast<float>(1920.0f), 0.0f, static_cast<float>(1080.0f));
    shader.use();
    glUniformMatrix4fv(glGetUniformLocation(shader.ID, "projection"), 1, GL_FALSE, glm::value_ptr(projection));




    //-------------------------------------------------------------------------------

    FT_Library ft;
    // All functions return a value different than 0 whenever an error occurred
    if (FT_Init_FreeType(&ft))
    {
        std::cout << "ERROR::FREETYPE: Could not init FreeType Library" << std::endl;
        return -1;
    }

    // find path to font
    //std::string font_name = FileSystem::getPath("resources/fonts/Antonio-Bold.ttf");
    std::string font_name = "E:/FTN-S7/Racunarska grafika/Vezbe 0/RG w0/Roboto-Black.ttf";

    if (font_name.empty())
    {
        std::cout << "ERROR::FREETYPE: Failed to load font_name" << std::endl;
        return -1;
    }

    // load font as face
    FT_Face face;
    if (FT_New_Face(ft, font_name.c_str(), 0, &face)) {
        std::cout << "ERROR::FREETYPE: Failed to load font" << std::endl;
        return -1;
    }
    else {
        // set size to load glyphs as
        FT_Set_Pixel_Sizes(face, 0, 48);

        // disable byte-alignment restriction
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

        // load first 128 characters of ASCII set
        for (unsigned char c = 0; c < 128; c++)
        {
            // Load character glyph 
            if (FT_Load_Char(face, c, FT_LOAD_RENDER))
            {
                std::cout << "ERROR::FREETYTPE: Failed to load Glyph" << std::endl;
                continue;
            }
            // generate texture
            unsigned int texture;
            glGenTextures(1, &texture);
            glBindTexture(GL_TEXTURE_2D, texture);
            glTexImage2D(
                GL_TEXTURE_2D,
                0,
                GL_RED,
                face->glyph->bitmap.width,
                face->glyph->bitmap.rows,
                0,
                GL_RED,
                GL_UNSIGNED_BYTE,
                face->glyph->bitmap.buffer
            );
            // set texture options
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            // now store character for later use
            Character character = {
                texture,
                glm::ivec2(face->glyph->bitmap.width, face->glyph->bitmap.rows),
                glm::ivec2(face->glyph->bitmap_left, face->glyph->bitmap_top),
                static_cast<unsigned int>(face->glyph->advance.x)
            };
            Characters.insert(std::pair<char, Character>(c, character));
        }
        glBindTexture(GL_TEXTURE_2D, 0);
    }
    // destroy FreeType once we're finished
    FT_Done_Face(face);
    FT_Done_FreeType(ft);

    // configure VAO/VBO for texture quads
    // -----------------------------------
    glGenVertexArrays(1, &textVAO);
    glGenBuffers(1, &textVBO);
    glBindVertexArray(textVAO);
    glBindBuffer(GL_ARRAY_BUFFER, textVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * 4, NULL, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    //-------------------------------------------------------------------------------
    unsigned int sandShader = createShader("basic.vert", "basic.frag");
    unsigned int sunShader = createShader("basic.vert", "sun.frag");
    unsigned int moonShader = createShader("basic.vert", "moon.frag");
    unsigned int pyramidShader = createShader("basic.vert", "basic.frag");
    unsigned int oazisShader = createShader("basic.vert", "oazis.frag");
    unsigned int fishShader = createShader("fish.vert", "basic.frag");
    unsigned int grassShader = createShader("basic.vert", "basic.frag");
    unsigned int name_lastname_indexShader = createShader("texture.vert", "texture.frag");
    unsigned int starShader = createShader("star.vert", "basic.frag");
    unsigned int doorShader = createShader("basic.vert", "basic.frag");
    unsigned int pyramidRightShader = createShader("pyramid.vert", "pyramid.frag");


    const int numberOfArraysBuffers = 11;

    unsigned int VAO[numberOfArraysBuffers];
    glGenVertexArrays(numberOfArraysBuffers, VAO);
    unsigned int VBO[numberOfArraysBuffers];
    glGenBuffers(numberOfArraysBuffers, VBO);

    
    //Sand //-------------------------------------------------------------------------------
    float r1 = 0.996078431372549;
    float g1 = 0.8588235294117647;
    float b1 = 0.6196078431372549;

    float r2 = 0.996078431372549;
    float g2 = 0.7215686274509804;
    float b2 = 0.4392156862745098;

    float r3 = 1.0;
    float g3 = 0.7568627450980392;
    float b3 = 0.47843137254901963;
    
    float r4 = 0.996078431372549;
    float g4 = 0.6941176470588235;
    float b4 = 0.40784313725490196;

    float vertices[] = {
        // Rectangle 1 (Topmost Sand Strip)
        -1.0f,  0.0f,    r1, g1, b1,  // Bottom-left
         1.0f,  0.0f,    r1, g1, b1,  // Bottom-right
        -1.0f,  0.25f,   r1, g1, b1,  // Top-left
         1.0f,  0.25f,   r1, g1, b1,  // Top-right

         // Rectangle 2
        -1.0f, -0.25f,   r2, g2, b2,  // Bottom-left
         1.0f, -0.25f,   r2, g2, b2,  // Bottom-right
        -1.0f,  0.0f,    r2, g2, b2,  // Top-left
         1.0f,  0.0f,    r2, g2, b2,  // Top-right

          // Rectangle 3
         -1.0f, -0.50f,   r3, g3, b3,  // Bottom-left
          1.0f, -0.50f,   r3, g3, b3,  // Bottom-right
         -1.0f, -0.25f,   r3, g3, b3,  // Top-left
          1.0f, -0.25f,   r3, g3, b3,  // Top-right

         // Rectangle 4 (Bottommost Sand Strip)
         -1.0f, -1.0f,    r4, g4, b4,  // Bottom-left
          1.0f, -1.0f,    r4, g4, b4,  // Bottom-righ
         -1.0f, -0.50f,   r4, g4, b4,  // Top-left
          1.0f, -0.50f,   r4, g4, b4,  // Top-right
    };

    glBindVertexArray(VAO[0]);
    glBindBuffer(GL_ARRAY_BUFFER, VBO[0]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);


    //The Sun and the Moon ---------------------------------------------------------------------
    float r = 0.1; 

    float sun[(CRES + 2) * 2];
    sun[0] = 0;
    sun[1] = 0.7;

    float moon[(CRES/2 + 2) * 2];
    moon[0] = 0;
    moon[1] = -0.7;

    for (int i = 0; i <= CRES; i++)
    {
        sun[2 + 2 * i] = (r-0.04) * cos((3.141592 / 180) * (i * 360 / CRES));
        sun[2 + 2 * i + 1] = r * sin((3.141592 / 180) * (i * 360 / CRES))+0.7;
    }

    for (int i = 0; i <= CRES/2; i++)
    {
        moon[2 + 2 * i] = (r - 0.04) * cos((3.141592 / 180) * (i * 360 / CRES));         
        moon[2 + 2 * i + 1] = r * sin((3.141592 / 180) * (i * 360 / CRES));
    }


    glBindVertexArray(VAO[1]);
    glBindBuffer(GL_ARRAY_BUFFER, VBO[1]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(sun), sun, GL_STATIC_DRAW);
    
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindVertexArray(VAO[6]);
    glBindBuffer(GL_ARRAY_BUFFER, VBO[6]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(moon), moon, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);


    //Pyramid ---------------------------------------------------------------------
    float triangleVertices[] = {
        // First triangle (center)
        -0.1f, -0.4f,  1.0f,  0.6f,  0.0f,
        -0.1f,  0.1f,  0.8f,  0.3f,  0.0f,
         0.2f, -0.4f,  1.0f,  0.6f,  0.0f,

        -0.1f, -0.4f,  0.8f,  0.3f,  0.0f,
        -0.1f,  0.1f,  1.0f,  0.6f,  0.0f,
        -0.4f, -0.4f,  0.8f,  0.3f,  0.0f,

         //Third triangle (left)
        -0.7f, -0.4f,  1.0f,  0.6f,  0.0f,
        -0.7f, -0.1f,  0.8f,  0.3f,  0.0f,
        -0.9f, -0.4f,  1.0f,  0.6f,  0.0f,

        -0.7f, -0.4f,  0.8f,  0.3f,  0.0f,
        -0.7f, -0.1f,  1.0f,  0.6f,  0.0f,
        -0.5f, -0.4f,  0.8f,  0.3f,  0.0f,


    };

    glBindVertexArray(VAO[2]);
    glBindBuffer(GL_ARRAY_BUFFER, VBO[2]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(triangleVertices), triangleVertices, GL_STATIC_DRAW);

    // Define vertex attribute pointers
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0); // Position attribute
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1); // Color attribute


    //Oazis ---------------------------------------------------------------------
    float ellipse[(CRES + 2) * 2]; // Array to hold ellipse vertex coordinates
    float radiusX = 0.3;          // Horizontal radius (semi-major axis)
    float radiusY = 0.17;           // Vertical radius (semi-minor axis)
    
    
    ellipse[0] = 0.5; // Center X0
    ellipse[1] = -0.7; // Center Y0

    int j;
    for (j = 0; j <= CRES; j++) {
        float angle = (3.141592 / 180) * (j * 360 / CRES); // Convert angle to radians
        ellipse[2 + 2 * j] = radiusX * cos(angle) + 0.5;         // X-coordinate
        ellipse[2 + 2 * j + 1] = radiusY * sin(angle) - 0.7;     // Y-coordinate
    }
    glBindVertexArray(VAO[3]); // Assuming VAO[2] is reserved for the ellipse
    glBindBuffer(GL_ARRAY_BUFFER, VBO[3]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(ellipse), ellipse, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

  
    //Fish ---------------------------------------------------------------------
    float fishVertices[] =
    {
        //tail
        0.5 + (0.12 * -0.3), -0.7 + (0.12 * 0.2),  0.9, 0.3, 0.9,
        0.5 + (0.12 * 0.1), -0.7 + (0.12 * 0.0),  0.5, 0.5, 0.9,
        0.5 + (0.12 * -0.3), -0.7 + (0.12 * -0.2),  0.9, 0.3, 0.9,

        // Body
        0.5 + (0.12 * 0.4), -0.7 + (0.12 * 0.2),  0.5, 0.5, 0.9,
        0.5 + (0.12 * 0.0), -0.7 + (0.12 * 0.0),  0.5, 0.5, 0.9,
        0.5 + (0.12 * 0.4), -0.7 + (0.12 * -0.2),  0.5, 0.5, 0.9,

        // Head
        0.5 + (0.12 * 0.4), -0.7 + (0.12 * 0.2),  0.5, 0.5, 0.9,
        0.5 + (0.12 * 0.7), -0.7 + (0.12 * 0.0),  0.5, 0.9, 0.9,
        0.5 + (0.12 * 0.4), -0.7 + (0.12 * -0.2),  0.5, 0.5, 0.9,
    };
    glBindVertexArray(VAO[4]);
    glBindBuffer(GL_ARRAY_BUFFER, VBO[4]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(fishVertices), fishVertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);


    //Grass ---------------------------------------------------------------------
    float grassVertices[] =
    {
        0.02 * 0.26,  0.0 * 0.26, 0.34 * 0.26, 0.63, 0.145,
        0.02 * 0.26,  0.7 * 0.26, 0.34 * 0.26, 0.63, 0.145,
        0.06 * 0.26, 0.0 * 0.26, 0.34 * 0.26, 0.63, 0.145,
        0.06 * 0.26, 0.7 * 0.26, 0.34 * 0.26, 0.63, 0.145,

        0.09 * 0.26, 0.0 * 0.26, 0.031 * 0.26, 0.5, 0.047,
        0.09 * 0.26, 0.8 * 0.26, 0.031 * 0.26, 0.5, 0.047,
        0.13 * 0.26, 0.0 * 0.26, 0.031 * 0.26, 0.5, 0.047,
        0.13 * 0.26, 0.8 * 0.26, 0.031 * 0.26, 0.5, 0.047,

        -0.01 * 0.26,  0.0 * 0.26, 0.2 * 0.26, 0.478, 0.21,
        -0.01 * 0.26,  0.9 * 0.26, 0.2 * 0.26, 0.478, 0.21,
        -0.07 * 0.26, 0.0 * 0.26, 0.2 * 0.26, 0.478, 0.21,
        -0.07 * 0.26, 0.9 * 0.26, 0.2 * 0.26, 0.478, 0.21,
    };



    glBindVertexArray(VAO[5]);
    glBindBuffer(GL_ARRAY_BUFFER, VBO[5]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(grassVertices), grassVertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);


    //Name, Lastname and Index texture ----------------------------------------------
    float indexVertices[] =
    {
        -0.95, -1.0, 0.0, 0.0,
        -0.95, -0.8, 0.0, 1.0,

        -0.6, -0.8, 1.0, 1.0,
        -0.6, -1.0, 1.0, 0.0
    };

    glBindVertexArray(VAO[7]);
    glBindBuffer(GL_ARRAY_BUFFER, VBO[7]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(indexVertices), indexVertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    unsigned checkerTexture = loadImageToTexture("texture_name_lastname_index.png"); //Ucitavamo teksturu
    glBindTexture(GL_TEXTURE_2D, checkerTexture); //Podesavamo teksturu
    glGenerateMipmap(GL_TEXTURE_2D); //Generisemo mipmape 
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);//S = U = X    GL_REPEAT, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_BORDER
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);// T = V = Y
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);   //GL_NEAREST, GL_LINEAR
    glBindTexture(GL_TEXTURE_2D, 0);

    unsigned uTexLoc = glGetUniformLocation(name_lastname_indexShader, "uTex");
    glUniform1i(uTexLoc, 0); // Indeks teksturne jedinice (sa koje teksture ce se citati boje)
    

    //Stars ---------------------------------------------------------------------
    float starVertices[] =
    {
        // Vertical bar
        -0.08f, -0.03f, 1.0f, 1.0f, 1.0f, // Bottom-left
        -0.07f, -0.03f, 1.0f, 1.0f, 1.0f, // Bottom-right
        -0.07f,  0.03f, 1.0f, 1.0f, 1.0f, // Top-right

        -0.08f, -0.03f, 1.0f, 1.0f, 1.0f, // Bottom-left
        -0.07f,  0.03f, 1.0f, 1.0f, 1.0f, // Top-right
        -0.08f,  0.03f, 1.0f, 1.0f, 1.0f, // Top-left

        // Horizontal bar
        -0.10f, -0.01f, 1.0f, 1.0f, 1.0f, // Bottom-left
        -0.05f, -0.01f, 1.0f, 1.0f, 1.0f, // Bottom-right
        -0.05f,  0.01f, 1.0f, 1.0f, 1.0f, // Top-right

        -0.10f, -0.01f, 1.0f, 1.0f, 1.0f, // Bottom-left
        -0.05f,  0.01f, 1.0f, 1.0f, 1.0f, // Top-right
        -0.10f,  0.01f, 1.0f, 1.0f, 1.0f, // Top-left
    };
    
    glBindVertexArray(VAO[8]);
    glBindBuffer(GL_ARRAY_BUFFER, VBO[8]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(starVertices), starVertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);


    float doorVertices[] =
    {
        // Center
        0.0f,  0.0f,  0.0f,  0.0f,  0.0f,  // bottom-left corner
        0.0f,  0.0f,  0.0f,  0.0f,  0.0f,  // top-left corner
        0.0f,  0.0f,  0.0f,  0.0f,  0.0f,  // bottom-right corner

          0.0f,  0.0f,  0.0f,  0.0f,  0.0f,  // bottom-left corner
        0.0f,  0.0f,  0.0f,  0.0f,  0.0f,  // top-left corner
        0.0f,  0.0f,  0.0f,  0.0f,  0.0f,  // bottom-right corner


         //Right
       0.0f,  0.0f,  0.0f,  0.0f,  0.0f,  // bottom-left corner
        0.0f,  0.0f,  0.0f,  0.0f,  0.0f,  // top-left corner
        0.0f,  0.0f,  0.0f,  0.0f,  0.0f,  // bottom-right corner

          0.0f,  0.0f,  0.0f,  0.0f,  0.0f,  // bottom-left corner
        0.0f,  0.0f,  0.0f,  0.0f,  0.0f,  // top-left corner
        0.0f,  0.0f,  0.0f,  0.0f,  0.0f,  // bottom-right corner


        //Left
        0.0f,  0.0f,  0.0f,  0.0f,  0.0f,  // bottom-left corner
        0.0f,  0.0f,  0.0f,  0.0f,  0.0f,  // top-left corner
        0.0f,  0.0f,  0.0f,  0.0f,  0.0f,  // bottom-right corner

          0.0f,  0.0f,  0.0f,  0.0f,  0.0f,  // bottom-left corner
        0.0f,  0.0f,  0.0f,  0.0f,  0.0f,  // top-left corner
        0.0f,  0.0f,  0.0f,  0.0f,  0.0f,  // bottom-right corner

    };


    glBindVertexArray(VAO[9]);
    glBindBuffer(GL_ARRAY_BUFFER, VBO[9]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(doorVertices), doorVertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);


    //Big pyramid
    float bigPyramidVertices[] =
    {
        // Second triangle (right)
        0.6f, -0.4f,  1.0f,  0.6f,  0.0f,
        0.6f,  0.2f,  0.8f,  0.3f,  0.0f,
        0.9f, -0.4f,  1.0f,  0.6f,  0.0f,

        0.6f, -0.4f,  0.8f,  0.3f,  0.0f,
        0.6f,  0.2f,  1.0f,  0.6f,  0.0f,
        0.3f, -0.4f,  0.8f,  0.3f,  0.0f,
    };

    glBindVertexArray(VAO[10]);
    glBindBuffer(GL_ARRAY_BUFFER, VBO[10]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(bigPyramidVertices), bigPyramidVertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
    // -------------------------------------------------------------------------------
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);


    // ++++++++++++++++++++++++++++++++++++++++++++++++++++++ RENDER LOOP - PETLJA ZA CRTANJE +++++++++++++++++++++++++++++++++++++++++++++++++

    glClearColor(0.5, 0.7, 1.0, 1.0); 
    vector<float> pyramidColors = {};
    //The Sun and the Moon rotation
    float angle = 0.0f; // Initial angle
    float rotationSpeed = 0.001f; // Speed of rotation
    float centerX = 0.0f, centerY = -0.2f; // Center of the circular path
    float radius = 0.95f; // Radius of the circular path
    bool isRotating = false;
    bool isNight = false;


    //Sky colors
    float red, green, blue;


    //Fish movement
    float xFishPos = -0.5;
    float yFishPos = 0.0;
    unsigned int uPosLoc = glGetUniformLocation(fishShader, "uPos");
    unsigned int uWLoc = glGetUniformLocation(fishShader, "uW");
    unsigned int uHLoc = glGetUniformLocation(fishShader, "uH");
    float swimSpeed = 0.0f;


    //Grass mechanics
    bool showGrass = true;


    //Doors
    float doorHeight = -0.4f;        // Height of the door (from 0 to 1)
    float doorSpeed = 0.00002f;        // Speed of the door animation (adjust as necessary)
    bool isDoorVisible = false;     // Flag to check if the door should be visible
    float doorMaxHeight = -0.3f;

    //Pyramid stripes
    bool seeStripes = false;

    //To be continued
    bool mousePressed = false;
    double mouseX, mouseY;

    while (!glfwWindowShouldClose(window))
    {
         if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
         {
            glfwSetWindowShouldClose(window, GL_TRUE);
         }
        
        glClear(GL_COLOR_BUFFER_BIT);

        //The Sun and the Moon
        sun[0] = centerX + radius * cos(angle);
        sun[1] = centerY + radius * sin(angle);

        moon[0] = centerX - radius * cos(angle);
        moon[1] = centerY - radius * sin(angle);

        for (int i = 0; i <= CRES; i++) {
            sun[2 + 2 * i] = sun[0] + (r - 0.04) * cos((3.141592 / 180) * (i * 360 / CRES));
            sun[2 + 2 * i + 1] = sun[1] + r * sin((3.141592 / 180) * (i * 360 / CRES));
        }

        for (int i = 0; i <= CRES / 2; i++) {
            float angles = (3.141592 / 180) * (i * 360 / CRES); // Angle for the current vertex

            // Original coordinates of the point
            float originalX = (r ) * cos(angles);
            float originalY = r * sin(angles) ;

            // Apply 90-degree rotation
            moon[2 + 2 * i] = moon[0] - originalY;  // Rotated x = -y
            moon[2 + 2 * i + 1] = moon[1] +originalX;  // Rotated y = x
        }

        if (glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS) {
            isRotating = false;
        }
        if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS) {
            isRotating = true;
            angle = 0;
        }

        if (isRotating) {
            angle += rotationSpeed;
            if (angle >= 2 * 3.141592) {
                angle -= 2 * 3.141592;
            }
        }
        else {
            angle = angle;
        }
        
        

        if (angle >= 0 && angle < 3.141592 / 2) {
            // Morning: Transition from reddish to blue
            red = 1.0f - (angle / (3.141592 / 2));  // Red decreases
            green = 0.5f + 0.5f * (angle / (3.141592 / 2));  // Green increases
            blue = 1.0f;  // Strong blue

            isNight = false;
        }
        else if (angle >= 3.141592 / 2 && angle < 3.141592) {
            // Evening: Transition from blue to reddish
            red = (angle - 3.141592 / 2) / (3.141592 / 2);  // Red increases
            green = 1.0f - 0.5f * ((angle - 3.141592 / 2) / (3.141592 / 2));  // Green decreases
            blue = 1.0f - ((angle - 3.141592 / 2) / (3.141592 / 2));  // Blue decreases
            
            isNight = false;
        }
        else {
            // Night: Transition to dark blue
            red = 0.0f;
            green = 0.0f;
            blue = 0.2f + 0.1f * cos((angle - 3.141592) / 3.141592);  // Slight fluctuation in blue

            isNight = true;
        }

        // Set the background color
        glClearColor(red, green, blue, 1.0f);
        glBindBuffer(GL_ARRAY_BUFFER, VBO[1]);
        glBufferData(GL_ARRAY_BUFFER, sizeof(sun), sun, GL_STATIC_DRAW);
        
        glViewport(0, 0, wWidth, wHeight);
        glUseProgram(sunShader);
        glBindVertexArray(VAO[1]);
        glDrawArrays(GL_TRIANGLE_FAN, 0, sizeof(sun) / (2 * sizeof(float)));

        glBindBuffer(GL_ARRAY_BUFFER, VBO[6]);
        glBufferData(GL_ARRAY_BUFFER, sizeof(moon), moon, GL_STATIC_DRAW);

        glViewport(0, 0, wWidth, wHeight);
        glUseProgram(moonShader);
        glBindVertexArray(VAO[6]);
        glDrawArrays(GL_TRIANGLE_FAN, 0, sizeof(moon) / (2 * sizeof(float)));


        //Sand
        glViewport(0, 0, wWidth, 3*wHeight/5); 
        glUseProgram(sandShader);
        glBindVertexArray(VAO[0]);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 16);
        

        //Stars
        if (isNight) {
            glViewport(0, 0, wWidth, wHeight);
            renderStars(10, starShader, VAO[8], wWidth, wHeight);
        }


        //Pyramids left and center
        glViewport(0, 0, wWidth,  wHeight);
        glUseProgram(pyramidShader); 
        glBindVertexArray(VAO[2]);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glDrawArrays(GL_TRIANGLES, 6, 6);

        //Pyramid right
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
            seeStripes = true;

        }


        float currentTime = glfwGetTime();  // Get the current time in seconds

        if (seeStripes) {
            glViewport(0, 0, wWidth, wHeight);
            glUseProgram(pyramidRightShader);

            // Pass the elapsed time to the shader
            glUniform1f(glGetUniformLocation(pyramidRightShader, "time"), currentTime);

            glBindVertexArray(VAO[10]);
            glDrawArrays(GL_TRIANGLES, 0, 6);
        }
        else {
            glViewport(0, 0, wWidth, wHeight);
            glUseProgram(pyramidShader);
            glBindVertexArray(VAO[10]);
            glDrawArrays(GL_TRIANGLES, 0, 6);
        }

        //Oazis
        glUseProgram(oazisShader);
        glBindVertexArray(VAO[3]);      
        glDrawArrays(GL_TRIANGLE_FAN, 0, CRES + 2); 


        //Fish
        if (glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS) {
            swimSpeed = 0.5;
            showGrass = false;
        }
        if (glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS) {
            swimSpeed = 0;
            showGrass = true;
        }
        xFishPos =   0.2f * sin(glfwGetTime() * swimSpeed);
        glUseProgram(fishShader);
        glBindVertexArray(VAO[4]);
        glUniform2f(uPosLoc, xFishPos, yFishPos);
        glUniform1i(uWLoc, wWidth);
        glUniform1i(uHLoc, wHeight);
        glDrawArrays(GL_TRIANGLES, 0, 9);


        //Grass
        if (showGrass) {
            glViewport(0, 0, wWidth + 300, wHeight - 700);
            glUseProgram(grassShader);
            glBindVertexArray(VAO[5]);
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
            glDrawArrays(GL_TRIANGLE_STRIP, 4, 4);
            glDrawArrays(GL_TRIANGLE_STRIP, 8, 4);

            glViewport(0, 0, wWidth + 800, wHeight - 550);
            glUseProgram(grassShader);
            glBindVertexArray(VAO[5]);
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
            glDrawArrays(GL_TRIANGLE_STRIP, 4, 4);
            glDrawArrays(GL_TRIANGLE_STRIP, 8, 4);

            glViewport(0, 0, wWidth + 1150, wHeight - 560);
            glUseProgram(grassShader);
            glBindVertexArray(VAO[5]);
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
            glDrawArrays(GL_TRIANGLE_STRIP, 4, 4);
            glDrawArrays(GL_TRIANGLE_STRIP, 8, 4);
        }
        

        //Name lastname index texture 
        glUseProgram(name_lastname_indexShader);
        glBindVertexArray(VAO[7]);
        glActiveTexture(GL_TEXTURE0); //tekstura koja se bind-uje nakon ovoga ce se koristiti sa SAMPLER2D uniformom u sejderu koja odgovara njenom indeksu
        glBindTexture(GL_TEXTURE_2D, checkerTexture);
        glViewport(0, 0, wWidth, wHeight);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        glBindTexture(GL_TEXTURE_2D, 0);

        

       

        //Black doors
        if (glfwGetKey(window, GLFW_KEY_O) == GLFW_PRESS) {
            isDoorVisible = true;
        }

        if (isDoorVisible) {
            if (doorHeight < doorMaxHeight) {
                doorHeight += doorSpeed;  // Increase the height gradually
            }

            // Update the door's vertices with the current height
            float doorVertices[] = {
                // Center pyramid door
                -0.13f, -0.4f  ,  0.0f,  0.0f,  0.0f,  // bottom-left corner
                -0.13f, doorHeight,  0.0f,  0.0f,  0.0f,  // top-left corner
                -0.07f, -0.4f ,  0.0f,  0.0f,  0.0f,  // bottom-right corner

                -0.13f,  doorHeight,  0.0f,  0.0f,  0.0f,  // top-left corner
                -0.07f,  doorHeight,  0.0f,  0.0f,  0.0f,  // top-right corner
                -0.07f, -0.4f ,  0.0f,  0.0f,  0.0f,  // bottom-right corner

                // Right pyramid door
                0.575f, -0.4f ,  0.0f,  0.0f,  0.0f,  // bottom-left corner
                0.575f,  doorHeight ,  0.0f,  0.0f,  0.0f,  // top-left corner
                0.625f, -0.4f,  0.0f,  0.0f,  0.0f,  // bottom-right corner

                0.575f, doorHeight,  0.0f,  0.0f,  0.0f,  // top-left corner
                0.625f, doorHeight,  0.0f,  0.0f,  0.0f,  // top-right corner
                0.625f, -0.4f ,  0.0f,  0.0f,  0.0f,  // bottom-right corner

                // Left door
                -0.715f, -0.4f,  0.0f,  0.0f,  0.0f,  // bottom-left corner
                -0.715f,  doorHeight ,  0.0f,  0.0f,  0.0f,  // top-left corner
                -0.685f, -0.4f ,  0.0f,  0.0f,  0.0f,  // bottom-right corner

                -0.715f,  doorHeight,  0.0f,  0.0f,  0.0f,  // top-left corner
                -0.685f,  doorHeight,  0.0f,  0.0f,  0.0f,  // top-right corner
                -0.685f, -0.4f ,  0.0f,  0.0f,  0.0f,  // bottom-right corner
            };

            glBindVertexArray(VAO[9]);
            glBindBuffer(GL_ARRAY_BUFFER, VBO[9]);
            glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(doorVertices), doorVertices);

            glViewport(0, 0, wWidth, wHeight);
            glUseProgram(doorShader);
            glBindVertexArray(VAO[9]);
            glDrawArrays(GL_TRIANGLES, 0, 18);


            //To be continued in 3D project
            glfwGetCursorPos(window, &mouseX, &mouseY);

            //Convert to normalized device coordinates
            float ndcX = (2.0f * mouseX) / wWidth - 1.0f;
            float ndcY = 1.0f - (2.0f * mouseY) / wHeight;
            if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
                
                if((ndcY >= -0.4 && ndcY <= doorHeight) && ((ndcX >= -0.13 && ndcX <= -0.07) || (ndcX >= 0.575 && ndcX <= 0.625) || (ndcX >= -0.715 && ndcX <= -0.685)))mousePressed = true;
            }

            if (mousePressed) {
                shader.use();
                RenderTextLetterByLetter(shader, "To be continued in 3D project!", 700.0f, 700.0, 0.8f, glm::vec3(0.0f, 0.0f, 0.0f), 0.2, window);
            }
            
        }
        
        
        //---------------------------------------------------
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }


    glfwTerminate();
    return 0;
}

unsigned int compileShader(GLenum type, const char* source)
{
    //Uzima kod u fajlu na putanji "source", kompajlira ga i vraca sejder tipa "type"
    //Citanje izvornog koda iz fajla
    std::string content = "";
    std::ifstream file(source);
    std::stringstream ss;
    if (file.is_open())
    {
        ss << file.rdbuf();
        file.close();
        std::cout << "Uspjesno procitao fajl sa putanje \"" << source << "\"!" << std::endl;
    }
    else {
        ss << "";
        std::cout << "Greska pri citanju fajla sa putanje \"" << source << "\"!" << std::endl;
    }
     std::string temp = ss.str();
     const char* sourceCode = temp.c_str(); //Izvorni kod sejdera koji citamo iz fajla na putanji "source"

    int shader = glCreateShader(type); //Napravimo prazan sejder odredjenog tipa (vertex ili fragment)
    
    int success; //Da li je kompajliranje bilo uspjesno (1 - da)
    char infoLog[512]; //Poruka o gresci (Objasnjava sta je puklo unutar sejdera)
    glShaderSource(shader, 1, &sourceCode, NULL); //Postavi izvorni kod sejdera
    glCompileShader(shader); //Kompajliraj sejder

    glGetShaderiv(shader, GL_COMPILE_STATUS, &success); //Provjeri da li je sejder uspjesno kompajliran
    if (success == GL_FALSE)
    {
        glGetShaderInfoLog(shader, 512, NULL, infoLog); //Pribavi poruku o gresci
        if (type == GL_VERTEX_SHADER)
            printf("VERTEX");
        else if (type == GL_FRAGMENT_SHADER)
            printf("FRAGMENT");
        printf(" sejder ima gresku! Greska: \n");
        printf(infoLog);
    }
    return shader;
}
unsigned int createShader(const char* vsSource, const char* fsSource)
{
    //Pravi objedinjeni sejder program koji se sastoji od Vertex sejdera ciji je kod na putanji vsSource

    unsigned int program; //Objedinjeni sejder
    unsigned int vertexShader; //Verteks sejder (za prostorne podatke)
    unsigned int fragmentShader; //Fragment sejder (za boje, teksture itd)

    program = glCreateProgram(); //Napravi prazan objedinjeni sejder program

    vertexShader = compileShader(GL_VERTEX_SHADER, vsSource); //Napravi i kompajliraj vertex sejder
    fragmentShader = compileShader(GL_FRAGMENT_SHADER, fsSource); //Napravi i kompajliraj fragment sejder

    //Zakaci verteks i fragment sejdere za objedinjeni program
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);

    glLinkProgram(program); //Povezi ih u jedan objedinjeni sejder program
    glValidateProgram(program); //Izvrsi provjeru novopecenog programa

    int success;
    char infoLog[512];
    glGetProgramiv(program, GL_VALIDATE_STATUS, &success); //Slicno kao za sejdere
    if (success == GL_FALSE)
    {
        glGetShaderInfoLog(program, 512, NULL, infoLog);
        std::cout << "Objedinjeni sejder ima gresku! Greska: \n";
        std::cout << infoLog << std::endl;
    }

    //Posto su kodovi sejdera u objedinjenom sejderu, oni pojedinacni programi nam ne trebaju, pa ih brisemo zarad ustede na memoriji
    glDetachShader(program, vertexShader);
    glDeleteShader(vertexShader);
    glDetachShader(program, fragmentShader);
    glDeleteShader(fragmentShader);

    return program;
}
static unsigned loadImageToTexture(const char* filePath) {
    int TextureWidth;
    int TextureHeight;
    int TextureChannels;
    unsigned char* ImageData = stbi_load(filePath, &TextureWidth, &TextureHeight, &TextureChannels, 0);
    if (ImageData != NULL)
    {
        //Slike se osnovno ucitavaju naopako pa se moraju ispraviti da budu uspravne
        stbi__vertical_flip(ImageData, TextureWidth, TextureHeight, TextureChannels);

        // Provjerava koji je format boja ucitane slike
        GLint InternalFormat = -1;
        switch (TextureChannels) {
        case 1: InternalFormat = GL_RED; break;
        case 2: InternalFormat = GL_RG; break;
        case 3: InternalFormat = GL_RGB; break;
        case 4: InternalFormat = GL_RGBA; break;
        default: InternalFormat = GL_RGB; break;
        }

        unsigned int Texture;
        glGenTextures(1, &Texture);
        glBindTexture(GL_TEXTURE_2D, Texture);
        glTexImage2D(GL_TEXTURE_2D, 0, InternalFormat, TextureWidth, TextureHeight, 0, InternalFormat, GL_UNSIGNED_BYTE, ImageData);
        glBindTexture(GL_TEXTURE_2D, 0);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        // oslobadjanje memorije zauzete sa stbi_load posto vise nije potrebna
        stbi_image_free(ImageData);
        return Texture;
    }
    else
    {
        std::cout << "Textura nije ucitana! Putanja texture: " << filePath << std::endl;
        stbi_image_free(ImageData);
        return 0;
    }
}
void renderStars(int count, GLuint starShader, GLuint VAO, int wWidth, int wHeight) {
    // Seed random number generator
    std::srand(static_cast<unsigned>(std::time(nullptr)));

    glUseProgram(starShader);

    // Loop to render each star
    for (int i = 0; i < count; ++i) {
        // Generate random positions
        float randomX = -1.0f + static_cast<float>(std::rand()) / (static_cast<float>(RAND_MAX / 2.0f)); // Range: [-1.0, 1.0]
        float randomY = -0.2f + static_cast<float>(std::rand()) / (static_cast<float>(RAND_MAX / 1.2f)); // Range: [-0.2, 1.0]

        // Create transformation matrix
        glm::mat4 transform = glm::mat4(1.0f); // Identity matrix
        transform = glm::translate(transform, glm::vec3(randomX, randomY, 0.0f)); // Translate

        // Pass transformation matrix to the shader
        GLuint transformLoc = glGetUniformLocation(starShader, "transform");
        glUniformMatrix4fv(transformLoc, 1, GL_FALSE, glm::value_ptr(transform));

        // Bind VAO and render star
        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLES, 0, 12);
    }
}
void RenderTextLetterByLetter(Shader& shader, std::string text, float x, float y, float scale, glm::vec3 color, float delay, GLFWwindow* window)
{
    static float lastTime = glfwGetTime(); // Track the last render time
    static size_t currentIndex = 0;        // Current character index

    // Check if enough time has passed to render the next letter
    float currentTime = glfwGetTime();
    if (currentTime - lastTime >= delay && currentIndex < text.size())
    {
        currentIndex++; // Move to the next character
        lastTime = currentTime; // Reset the timer
    }

    // Activate rendering state
    shader.use();
    glUniform3f(glGetUniformLocation(shader.ID, "textColor"), color.x, color.y, color.z);
    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(textVAO);

    float currentX = x; // Track x position for each character
    for (size_t i = 0; i < currentIndex; ++i)
    {
        Character ch = Characters[text[i]];

        float xpos = currentX + ch.Bearing.x * scale;
        float ypos = y - (ch.Size.y - ch.Bearing.y) * scale;

        float w = ch.Size.x * scale;
        float h = ch.Size.y * scale;

        // Update VBO for the current character
        float vertices[6][4] = {
            { xpos,     ypos + h,   0.0f, 0.0f },
            { xpos,     ypos,       0.0f, 1.0f },
            { xpos + w, ypos,       1.0f, 1.0f },

            { xpos,     ypos + h,   0.0f, 0.0f },
            { xpos + w, ypos,       1.0f, 1.0f },
            { xpos + w, ypos + h,   1.0f, 0.0f }
        };

        glBindTexture(GL_TEXTURE_2D, ch.TextureID);
        glBindBuffer(GL_ARRAY_BUFFER, textVBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        // Render quad
        glDrawArrays(GL_TRIANGLES, 0, 6);

        // Advance cursor for next glyph
        currentX += (ch.Advance >> 6) * scale;
    }

    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);

    

    if (currentIndex == text.size() && currentTime - lastTime >= 2.0f)
    {
        
        glfwSetWindowShouldClose(window, GL_TRUE); // Assuming `window` is your GLFW window object
    }

   
}
void RenderTextLetterByLetterWithFade(Shader& shader, std::string text, float x, float y, float scale, glm::vec3 color, float delay, float fadeDuration)
{
    static float lastTime = glfwGetTime(); // Track the last render time
    static size_t currentIndex = 0;        // Current character index
    static std::vector<float> fadeStartTimes(text.size(), -1.0f); // Track fade start times for each letter

    float currentTime = glfwGetTime();

    // Check if enough time has passed to render the next letter
    if (currentTime - lastTime >= delay && currentIndex < text.size())
    {
        fadeStartTimes[currentIndex] = currentTime; // Mark fade start time for the current letter
        currentIndex++; // Move to the next character
        lastTime = currentTime; // Reset the timer
    }

    // Activate rendering state
    shader.use();
    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(textVAO);

    float currentX = x; // Track x position for each character
    for (size_t i = 0; i < currentIndex; ++i)
    {
        Character ch = Characters[text[i]];

        float xpos = currentX + ch.Bearing.x * scale;
        float ypos = y - (ch.Size.y - ch.Bearing.y) * scale;

        float w = ch.Size.x * scale;
        float h = ch.Size.y * scale;

        // Calculate fade-out alpha for this letter
        float alpha = 1.0f; // Default fully visible
        if (fadeStartTimes[i] > 0) // Only fade letters that have a start time
        {
            float fadeElapsed = currentTime - fadeStartTimes[i];
            if (fadeElapsed > fadeDuration)
            {
                alpha = 0.0f; // Fully faded
            }
            else
            {
                alpha = 1.0f - (fadeElapsed / fadeDuration); // Linear fade-out
            }
        }

        // Apply text color with alpha
        glm::vec3 fadedColor = color * alpha;
        glUniform3f(glGetUniformLocation(shader.ID, "textColor"), fadedColor.x, fadedColor.y, fadedColor.z);

        // Update VBO for the current character
        float vertices[6][4] = {
            { xpos,     ypos + h,   0.0f, 0.0f },
            { xpos,     ypos,       0.0f, 1.0f },
            { xpos + w, ypos,       1.0f, 1.0f },

            { xpos,     ypos + h,   0.0f, 0.0f },
            { xpos + w, ypos,       1.0f, 1.0f },
            { xpos + w, ypos + h,   1.0f, 0.0f }
        };

        glBindTexture(GL_TEXTURE_2D, ch.TextureID);
        glBindBuffer(GL_ARRAY_BUFFER, textVBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        // Render quad
        glDrawArrays(GL_TRIANGLES, 0, 6);

        // Advance cursor for next glyph
        currentX += (ch.Advance >> 6) * scale;
    }

    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);

    // Reset state if all letters have faded out
    if (currentIndex == text.size() && fadeStartTimes.back() > 0 && currentTime - fadeStartTimes.back() > fadeDuration)
    {
        currentIndex = 0;                          // Reset rendering
        std::fill(fadeStartTimes.begin(), fadeStartTimes.end(), -1.0f); 
    }
    
}