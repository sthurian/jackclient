#include <iostream>

#include "jackclient/jackclient.h"

class ExampleClient : public JackClient {
   private:
    std::unique_ptr<JackAudioInputPort> inPort;
    std::unique_ptr<JackAudioOutputPort> outPort;

   public:
    ExampleClient() : JackClient("test") {
        open();
        this->inPort = this->createAudioInputPort("example_input");
        this->outPort = this->createAudioOutputPort("example_output");
        activate();
        for (auto &in : this->getAudioInputPorts()) {
            if (in->isPhysical()) {
                in->connectTo(*outPort);
            }
        }
        for (auto &out : this->getAudioOutputPorts()) {
            if (out->isPhysical()) {
                out->connectTo(*inPort);
            }
        }
    };
    ~ExampleClient() { this->close(); }

   protected:
    int onProcess(uint32_t sampleCount) {
        float *out = outPort->getBuffer();
        float *in = inPort->getBuffer();
        for (uint32_t i = 0; i < sampleCount; i++) (*out++) = in[i];
        return 0;
    }
};
int main() {
    try {
        ExampleClient client;
        char input;
        std::cout << "\nRunning ... press <enter> to quit" << std::endl;
        std::cin.get(input);
    } catch (JackClientException &e) {
        std::cerr << e.what() << std::endl;
    }
    return 0;
}
