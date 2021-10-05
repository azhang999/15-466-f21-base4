#include "Mode.hpp"

#include "Scene.hpp"
#include "Sound.hpp"

#include <glm/glm.hpp>

#include <vector>
#include <deque>

enum StateType {DIALOGUE, CHOICE, EFFECT};

struct PlayMode : Mode {
	PlayMode();
	virtual ~PlayMode();

	//functions called by main loop:
	virtual bool handle_event(SDL_Event const &, glm::uvec2 const &window_size) override;
	virtual void update(float elapsed) override;
	virtual void draw(glm::uvec2 const &drawable_size) override;

    void render_text(std::string text, float x, float y, float scale, glm::vec3 color, glm::uvec2 const &drawable_size);
    float render_line(std::string text, float &start_x, float &start_y, float scale, glm::vec3 color, glm::uvec2 const &drawable_size);
	//----- game state -----
    bool up_pressed = false;
    bool down_pressed = false;
    bool enter_pressed = false;

    struct Dialogue {
        uint32_t id = 0;
        std::string text;
    };

    struct ChoiceSelect {
        float prob = 0.f;
        uint32_t effect1_id = 0;
        uint32_t effect2_id = 0;
        std::string text;
    };

    struct Choice {
        uint32_t id = 0;
        std::string text;

        ChoiceSelect choice1;
        ChoiceSelect choice2;
    };

    struct Effect {
        uint32_t id = 0;
        std::string text;
        int32_t academics = 0;
        int32_t social = 0;
        int32_t health = 0;
    };
    std::unordered_map<uint32_t, StateType> id_to_state_type;
    std::unordered_map<uint32_t, Dialogue> dialogue_map;
    std::unordered_map<uint32_t, Choice> choice_map;
    std::unordered_map<uint32_t, Effect> effect_map;

    void read_dialogue();
    void read_choice();
    void read_choice_select(std::ifstream &file, ChoiceSelect &choice);
    void read_effect();

    uint32_t PASS = 4;
    uint32_t FAIL_ACADEMICS = 1;
    uint32_t FAIL_SOCIAL = 2;
    uint32_t FAIL_HEALTH = 3;

    uint32_t story_line [25] = {
    0, 
    5, 0, 
    33, 0,
    9, 0, 
    41, 0,
    25, 0,
    37, 0,
    13, 0, 
    29, 0,
    21, 0, 
    17, 0, 
    PASS, FAIL_ACADEMICS, FAIL_SOCIAL, FAIL_HEALTH};
    uint32_t current_event = 0;

    int32_t academics = 100;
    int32_t social = 100;
    int32_t health = 100;

    float align_left_x = 100.f;
    float dialogue_y_minus = 200.f;

    float choice1_y;
    float choice2_y;

    bool choice1_selected = true;

    // float stats_x;
    float stats_y = 100.f;

};
