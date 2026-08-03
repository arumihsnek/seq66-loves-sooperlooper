#include <iostream>
#include "audio/sooperlooper_audio_slot_widget.hpp"
int main() {
    seq66::audio_slot_model m;
    std::cout << "label: " << m.state_label() << std::endl;
    std::cout << "cmd: " << m.command_label() << std::endl;
    return 0;
}
