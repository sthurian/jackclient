#include <iostream>
#include "jackclient/jackclient.h"

class ExampleClient : public JackClient
{
    private:
        JackAudioInputPort* inPort = nullptr;
        JackAudioOutputPort* outPort = nullptr;
    public:
        ExampleClient(): JackClient("test"){
            open();
            inPort = (JackAudioInputPort*) this->createInputPort("example_input");
            outPort = (JackAudioOutputPort*) this->createOutputPort("example_output");
            activate();
            for(JackInputPort* in: listInputPorts(JackPortType::Audio,true))
            {
                in->connectTo(outPort);
                delete in;
            }
            for(JackOutputPort* out: listOutputPorts(JackPortType::Audio,true))
            {
                out->connectTo(inPort);
                delete out;
            }
        };
        ~ExampleClient()
        {
            delete inPort;
            delete outPort;
            close();
        }
    protected:
        int onProcess(uint32_t sampleCount)
        {
            float* out = outPort->getBuffer();
            float* in= inPort->getBuffer();
            for(uint32_t i=0;i<sampleCount; i++)
                (*out++)=in[i];
            return 0;
        }

};
int main()
{

    try
    {
        ExampleClient client;
        char input;

        std::cout << "\nRunning ... press <enter> to quit" << std::endl;
        std::cin.get(input);
    } catch (JackClientException& e)
    {
        std::cerr << e.what() << std::endl;
    }

	return 0;
}