#include "PlayMode.hpp"

#include "ColorTextureProgram.hpp"
#include "Mesh.hpp"
#include "Load.hpp"
#include "gl_errors.hpp"
#include "data_path.hpp"

#include <glm/gtc/type_ptr.hpp>
#include <hb.h>
#include <hb-ft.h>
#include <ft2build.h>
#include FT_FREETYPE_H

#include <random>
#include <fstream>

#define FONT_SIZE 36
#define MARGIN (FONT_SIZE * .5)

// SOURCE for Character struct idea: https://learnopengl.com/In-Practice/Text-Rendering
/// Holds all state information relevant to a character as loaded using FreeType
struct Character {
    unsigned int TextureID; // ID handle of the glyph texture
    glm::ivec2   Size;      // Size of glyph
    glm::ivec2   Bearing;   // Offset from baseline to left/top of glyph
    unsigned int Advance;   // Horizontal offset to advance to next glyph
};
std::map<hb_codepoint_t, Character> Characters;
unsigned int VAO, VBO;

void PlayMode::read_dialogue() {
    std::string dialogue_path = data_path("dialogue.txt");
    std::ifstream file(dialogue_path);
    std::string id_line;
    while (std::getline(file, id_line)) {
        // parse out id
        uint32_t id = std::stoi(id_line);
        std::string text;
        std::string line = ".";
        while (line != "") {
            // txt file needs empty line at end
            std::getline(file, line);
            text.append(line + "\n");
        }
        // build dialogue
        Dialogue dialogue{id, text};
        dialogue_map[id] = dialogue;
        id_to_state_type[id] = DIALOGUE;
    }
}

void PlayMode::read_choice_select(std::ifstream &file, ChoiceSelect &choice) {
    std::string line = "";
    // get probability
    std::getline(file, line);
    choice.prob = std::stof(line);

    // get effect1 id
    line = "";
    std::getline(file, line);
    choice.effect1_id = std::stoi(line);

    // get effect2 id
    line = "";
    std::getline(file, line);
    choice.effect2_id = std::stoi(line);

    // get text - choice can only be 1 line
    line = "";
    std::getline(file, line);
    choice.text = line;
}

void PlayMode::read_choice() {
    std::string choice_path = data_path("choice.txt");
    std::ifstream file(choice_path);
    std::string id_line;
    while (std::getline(file, id_line)) {
        // parse out id
        uint32_t id = std::stoi(id_line);
        Choice choice;
        choice.id = id;
        read_choice_select(file, choice.choice1);
        read_choice_select(file, choice.choice2);

        std::string text;
        std::string line = ".";
        while (line != "") {
            // txt file needs empty line at end
            std::getline(file, line);
            text.append(line + "\n");
        }
        // build dialogue
        choice.text = text;
        choice_map[id] = choice;
        id_to_state_type[id] = CHOICE;
    }
}

void PlayMode::read_effect() {
    std::string effect_path = data_path("effect.txt");
    std::ifstream file(effect_path);
    std::string id_line;
    while (std::getline(file, id_line)) {
        // parse out id
        uint32_t id = std::stoi(id_line);
        Effect effect;
        effect.id = id;
    
        std::string stat_line;
        
        std::getline(file, stat_line);
        effect.academics = std::stoi(stat_line);
        std::getline(file, stat_line);
        effect.social = std::stoi(stat_line);
        std::getline(file, stat_line);
        effect.health = std::stoi(stat_line);

        std::string text;
        std::string line = ".";
        while (line != "") {
            // txt file needs empty line at end
            std::getline(file, line);
            text.append(line + "\n");
        }
        // build dialogue
        effect.text = text;
        effect_map[id] = effect;
        id_to_state_type[id] = EFFECT;
    }
}

FT_Library ft;
FT_Face face;
PlayMode::PlayMode() {
    // SOURCE for initializing opengl + FT: https://learnopengl.com/In-Practice/Text-Rendering
    // OpenGL state
    // ------------
    glEnable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    if (FT_Init_FreeType(&ft))
    {
        std::cout << "ERROR::FREETYPE: Could not init FreeType Library" << std::endl;
        abort();
    }

	// find path to font
    std::string font_name = data_path("Arial.ttf");
    if (font_name.empty())
    {
        std::cout << "ERROR::FREETYPE: Failed to load font_name" << std::endl;
        abort();
    }
	
	// load font as face
    // FT_Face face;
    if (FT_New_Face(ft, font_name.c_str(), 0, &face)) {
        std::cout << "ERROR::FREETYPE: Failed to load font" << std::endl;
        abort();
    }

    // configure VAO/VBO for texture quads
    // -----------------------------------
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * 4, NULL, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    read_dialogue();
    read_choice();
    read_effect();
}

PlayMode::~PlayMode() {
}

bool PlayMode::handle_event(SDL_Event const &evt, glm::uvec2 const &window_size) {
    if (evt.type == SDL_KEYDOWN) {
        if (evt.key.keysym.sym == SDLK_UP) {
            up_pressed = true;
            return true;
        } else if (evt.key.keysym.sym == SDLK_DOWN) {
            down_pressed = true;
            return true;
        } else if (evt.key.keysym.sym == SDLK_RETURN) {
            enter_pressed = true;
            return true;
        }
    }

	return false;
}

void PlayMode::update(float elapsed) {
    uint32_t state_id = story_line[current_event];
    StateType state_type = id_to_state_type[state_id];

    // reached end of game
    if (current_event >= 11 && enter_pressed) {
        current_event = 0;
        academics = 100;
        social = 100;
        health = 100;
    } else if (academics <= 0 && enter_pressed) {
        current_event = 12;
    } else if (social <= 0 && enter_pressed) {
        current_event = 13;
    } else if (health <= 0 && enter_pressed) {
        current_event = 14;
    } else if (state_type == DIALOGUE && enter_pressed) {
        current_event++;
    } else if (state_type == CHOICE) {
        if (up_pressed) {
            choice1_selected = true;
        } else if (down_pressed) {
            choice1_selected = false;
        }

        // go to effect
        if (enter_pressed) {
            Choice choice = choice_map[state_id];
            ChoiceSelect choice_selected;
            if (choice1_selected) {
                choice_selected = choice.choice1;
            } else {
                choice_selected = choice.choice2;
            }

            float p = static_cast <float> (rand()) /(static_cast <float> (RAND_MAX));
            Effect effect;
            if (p < choice_selected.prob) {
                effect = effect_map[choice_selected.effect1_id];
            } else {
                effect = effect_map[choice_selected.effect2_id];
            }

            academics += effect.academics;
            social += effect.social;
            health += effect.health;
            current_event++;
            story_line[current_event] = effect.id;
        }
    } else if (state_type == EFFECT && enter_pressed) {
        current_event++;
        choice1_selected = true;
    }

    up_pressed = false;
    down_pressed = false;
    enter_pressed = false;
}

// SOURCE for most of render_line function: https://learnopengl.com/In-Practice/Text-Rendering
float PlayMode::render_line(std::string text, float &start_x, float &start_y, float scale, glm::vec3 color, glm::uvec2 const &drawable_size) {

    glDisable(GL_DEPTH_TEST);
    glUseProgram(color_texture_program->program);
    glm::mat4 projection = glm::ortho(0.0f, (float)drawable_size.x, 0.0f, (float)drawable_size.y);
    glUniformMatrix4fv(color_texture_program->OBJECT_TO_CLIP_mat4, 1, GL_FALSE, glm::value_ptr(projection));
    glUniform3f(color_texture_program->Color_vec3, color.x, color.y, color.z);

    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(VAO);

    // SOURCE for setting up harfbuzz: https://github.com/harfbuzz/harfbuzz-tutorial/blob/master/hello-harfbuzz-freetype.c
    /* Create hb-ft font. */
    hb_font_t *hb_font;
    hb_font = hb_ft_font_create (face, NULL);

    /* Create hb-buffer and populate. */
    hb_buffer_t *hb_buffer;
    hb_buffer = hb_buffer_create ();
    hb_buffer_add_utf8 (hb_buffer, text.c_str(), -1, 0, -1);
    hb_buffer_guess_segment_properties (hb_buffer);

    /* Shape it! */
    hb_shape (hb_font, hb_buffer, NULL, 0);

    /* Get glyph information and positions out of the buffer. */
    unsigned int len = hb_buffer_get_length (hb_buffer);
    hb_glyph_info_t *info = hb_buffer_get_glyph_infos (hb_buffer, NULL);
    hb_glyph_position_t *pos = hb_buffer_get_glyph_positions (hb_buffer, NULL);

    float x = start_x;
    float y = start_y;
    float biggest_char_size = 0;

    float enter = 0;

    // create new charcters for the info[i].codepoints that are not already loaded
    for (unsigned int i = 0; i < len; ++i) {
        auto codepoint = info[i].codepoint;

        auto it = Characters.find(codepoint);
        Character ch;
        if (it == Characters.end()) {
             // set size to load glyphs as
            FT_Set_Pixel_Sizes(face, 0, 48);

            // disable byte-alignment restriction
            glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

            if (FT_Load_Glyph(face, codepoint, FT_LOAD_RENDER)) {
                std::cout << "ERROR::FREETYTPE: Failed to load Glyph (FT_Load_Glyph)" << std::endl;
                abort();
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
            ch = {
                texture,
                glm::ivec2(face->glyph->bitmap.width, face->glyph->bitmap.rows),
                glm::ivec2(face->glyph->bitmap_left, face->glyph->bitmap_top),
                static_cast<unsigned int>(face->glyph->advance.x)
            };
            Characters.insert(std::pair<hb_codepoint_t, Character>(codepoint, ch));
        } else {
            ch = it->second;
        }

        biggest_char_size = (float)fmax(biggest_char_size, ch.Size.y);

        float xpos = x + ch.Bearing.x * scale;
        float ypos = y - (ch.Size.y - ch.Bearing.y) * scale;

        float w = ch.Size.x * scale;
        float h = ch.Size.y * scale;

        // update VBO for each character
        float vertices[6][4] = {
            { xpos,     ypos + h,   0.0f, 0.0f },            
            { xpos,     ypos,       0.0f, 1.0f },
            { xpos + w, ypos,       1.0f, 1.0f },

            { xpos,     ypos + h,   0.0f, 0.0f },
            { xpos + w, ypos,       1.0f, 1.0f },
            { xpos + w, ypos + h,   1.0f, 0.0f }           
        };
        // render glyph texture over quad
        glBindTexture(GL_TEXTURE_2D, ch.TextureID);
        // update content of VBO memory
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices); // be sure to use glBufferSubData and not glBufferData

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        // render quad
        glDrawArrays(GL_TRIANGLES, 0, 6);
        // now advance cursors for next glyph (note that advance is number of 1/64 pixels)
        x += (pos[i].x_advance >> 6) * scale; // bitshift by 6 to get value in pixels (2^6 = 64 (divide amount of 1/64th pixels by 64 to get amount of pixels))
        y += (pos[i].y_advance >> 6) * scale;

        if (x >= ((float)drawable_size.x - 50.0f)) {
            x = start_x;
            y -= (biggest_char_size * 2 * scale);
        }
        start_y = y;
    }

    enter = biggest_char_size * 2 * scale;
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
    glUseProgram(0);
    return enter;
}

void PlayMode::render_text(std::string text, float start_x, float start_y, float scale, glm::vec3 color, glm::uvec2 const &drawable_size) {
    std::string line;
    float x = start_x;
    float y = start_y;
    for (auto &ch : text) {
        if (ch == '\n') {
            if (line.size() > 0) {
                float enter = render_line(line, x, y, scale, color, drawable_size);
                y -= enter;
            }
            line = "";
        } else {
            line.push_back(ch);
        }
    }
    if (line.size() > 0) {
        render_line(line, x, y, scale, color, drawable_size);
    }
}

void PlayMode::draw(glm::uvec2 const &drawable_size) {
	glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
	glClearDepth(1.0f); //1.0 is actually the default value to clear the depth buffer to, but FYI you can change it.
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    uint32_t state_id = story_line[current_event];
    StateType state_type = id_to_state_type[state_id];

    float scale = drawable_size.x/2560.f;
    scale = fmin(scale, drawable_size.y/1440.f);

    switch (state_type) {
        case DIALOGUE: {
            Dialogue dialogue = dialogue_map[state_id];
            // render dialogue
            render_text(dialogue.text, align_left_x, drawable_size.y - dialogue_y_minus * scale, scale, glm::vec3(0.0f, 0.0f, 0.0f), drawable_size);
            break;
        }
        case CHOICE: {
            Choice choice = choice_map[state_id];
            // render text
            render_text(choice.text, align_left_x, drawable_size.y - dialogue_y_minus * scale, scale, glm::vec3(0.0f, 0.0f, 0.0f), drawable_size);

            if (choice1_selected) {
                std::string chosen_text = "[ " + choice.choice1.text + " ]";
                render_text(chosen_text, align_left_x, drawable_size.y - dialogue_y_minus* 3 *scale, scale, glm::vec3(0.0f, 0.0f, 0.0f), drawable_size);
                render_text(choice.choice2.text, align_left_x, drawable_size.y - dialogue_y_minus * 4 * scale, scale, glm::vec3(0.0f, 0.0f, 0.0f), drawable_size);
            } else {
                std::string chosen_text = "[ " + choice.choice2.text + " ]";
                render_text(choice.choice1.text, align_left_x, drawable_size.y - dialogue_y_minus*3 * scale, scale, glm::vec3(0.0f, 0.0f, 0.0f), drawable_size);
                render_text(chosen_text, align_left_x, drawable_size.y - dialogue_y_minus*4 * scale, scale, glm::vec3(0.0f, 0.0f, 0.0f), drawable_size);
            }

            break;
        }
        case EFFECT: {
            Effect effect = effect_map[state_id];
            // render dialogue
            render_text(effect.text, align_left_x, drawable_size.y - dialogue_y_minus * scale, scale, glm::vec3(0.0f, 0.0f, 0.0f), drawable_size);
            break;
        }
    }

    if (academics > 100) {
        academics = 100;
    }

    if (social > 100) {
        social = 100;
    }

    if (health > 100) {
        health = 100;
    }

    // render stats
    std::string stats = "Academics: " + std::to_string(academics) + "   Social: " + std::to_string(social) + "   Health: " + std::to_string(health);
    render_text(stats, align_left_x, drawable_size.y - dialogue_y_minus*6*scale, scale, glm::vec3(0.0f, 0.0f, 0.0f), drawable_size);

	GL_ERRORS();
}