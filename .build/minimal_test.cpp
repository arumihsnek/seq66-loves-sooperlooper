#include <iostream>
#include "audio/sooperlooper_audio_slot_state_renderer.hpp"
int main() {
    std::cout << "Creating renderer..." << std::endl;
    seq66::sooperlooper_audio_slot_state_renderer renderer;
    std::cout << "Creating model..." << std::endl;
    seq66::audio_slot_model model;
    std::cout << "Rendering..." << std::endl;
    auto result = renderer.render(model, 1000);
    std::cout << "Display: " << result.display_label() << std::endl;
    std::cout << "OK" << std::endl;
    return 0;
}
