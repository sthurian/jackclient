#ifndef _JACKCLIENT_H
#define _JACKCLIENT_H
#include <jack/jack.h>
#include <jack/midiport.h>

#include <memory>
#include <string>
#include <vector>

enum class JackPortType { AUDIO, MIDI };
enum class JackState { ACTIVE, INACTIVE, CLOSED };
namespace Transport {
enum class JackTransportState {
    STARTING = JackTransportStarting,
    ROLLING = JackTransportRolling,
    STOPPED = JackTransportStopped
};
using JackPosition = jack_position_t;
}  // namespace Transport

class JackClient;
/**
 *
 *
 *
 * */
class JackClientException : public std::exception {
   private:
    std::string message;

   public:
    JackClientException(std::string message) : message(message){};
    ~JackClientException(){};
    virtual const char* what() { return message.c_str(); };
};
/**
 *
 *
 *
 * */
class JackPort {
   protected:
    JackClient* client;
    JackPortType portType;
    jack_client_t* client_handle;
    uint32_t bufSize;
    jack_port_t* port;
    JackPort(JackClient* client, const char* name, JackPortType type);
    JackPort(JackClient* client, jack_port_t* port, JackPortType type);
    void* getBufferInternal();
    template <typename T>
    std::vector<std::unique_ptr<T>> listConnections();

   public:
    ~JackPort();
    JackPortType getPortType();
    std::string getName();
    void disconnectAll();
    std::string getShortName();
    bool isPhysical();
    bool isMine();
};
/**
 *
 *
 *
 * */
class JackOutputPort;
class JackInputPort : public JackPort {
    friend class JackOutputPort;

   protected:
    JackInputPort(JackClient* client, const char* name, JackPortType type);
    JackInputPort(JackClient* client, jack_port_t* port, JackPortType type);

   public:
    void connectTo(JackOutputPort& port);
    void disconnect(JackOutputPort& port);
};
/**
 *
 *
 *
 * */
class JackOutputPort : public JackPort {
    friend class JackInputPort;

   protected:
    JackOutputPort(JackClient* client, const char* name, JackPortType type);
    JackOutputPort(JackClient* client, jack_port_t* port, JackPortType type);

   public:
    void connectTo(JackInputPort& port);
    void disconnect(JackInputPort& port);
};

class JackAudioOutputPort;
class JackAudioInputPort : public JackInputPort {
    friend class JackAudioOutputPort;

   public:
    JackAudioInputPort(JackClient* client, const char* name);
    JackAudioInputPort(JackClient* client, jack_port_t* port);
    std::vector<std::unique_ptr<JackAudioOutputPort>> getConnections();
    float* getBuffer();
};
class JackAudioOutputPort : public JackOutputPort {
    friend class JackAudioInputPort;

   public:
    JackAudioOutputPort(JackClient* client, const char* name);
    JackAudioOutputPort(JackClient* client, jack_port_t* port);
    std::vector<std::unique_ptr<JackAudioInputPort>> getConnections();
    float* getBuffer();
};

class JackMIDIEvent {
   private:
    uint32_t time;
    size_t size;
    unsigned char* buffer;

   public:
    JackMIDIEvent(uint32_t time, size_t size, unsigned char* buffer);
    uint32_t getTime();
    size_t getSize();
    unsigned char* getMIDIData();
};

class JackMIDIInputPort : public JackInputPort {
    friend class JackOutputPort;

   public:
    JackMIDIInputPort(JackClient* client, const char* name);
    JackMIDIInputPort(JackClient* client, jack_port_t* port);
    std::vector<std::unique_ptr<JackMIDIEvent>> getMIDIEvents();
    ~JackMIDIInputPort();
};

class JackMIDIOutputPort : public JackOutputPort {
    friend class JackInputPort;

   public:
    JackMIDIOutputPort(JackClient* client, const char* name);
    JackMIDIOutputPort(JackClient* client, jack_port_t* port);
    void clearBuffer();
    void writeEvent(JackMIDIEvent& event);
    void write(uint32_t time, unsigned char* buffer, size_t size);
};

/**
 *
 *
 *
 * */
class JackClient {
    friend class JackPort;

   private:
    JackState jackState = JackState::CLOSED;
    jack_status_t status;
    jack_client_t* client;
    static jack_nframes_t nframes;
    const char* name;
    static int process(jack_nframes_t nframes, void* arg);
    static void jack_shutdown(void* arg);
    static void handleShutdown(void* arg);
    static int buffer_size_callback(jack_nframes_t nframes, void* arg);
    static int xrun_callback(void* arg);
    static int sync_callback(jack_transport_state_t state, jack_position_t* pos, void* arg);
    static void timebase_callback(jack_transport_state_t state, jack_nframes_t nframes,
                                  jack_position_t* pos, int new_pos, void* arg);
    template <typename T>
    std::vector<std::unique_ptr<T>> createPorts(JackPortType type, JackPortFlags flags);

   protected:
    virtual int onProcess(uint32_t sampleCount) = 0;
    virtual void onShutdown(){};
    virtual void onXRun(){};
    virtual int onTransportStart(Transport::JackPosition* pos) { return 1; };
    virtual int onTransportStop(Transport::JackPosition* pos) { return 1; };
    virtual int onTransportRoll(Transport::JackPosition* pos) { return 1; };
    virtual int onTransportSync(Transport::JackTransportState state, Transport::JackPosition* pos);
    virtual void onTimebase(Transport::JackTransportState state, uint32_t frame,
                            Transport::JackPosition* pos, bool newPos){};

   public:
    JackState getState();
    JackClient(const char* name);
    ~JackClient();
    void open();
    void close();
    void activate();
    void startTransport();
    void stopTransport();
    void setTransportPosition(uint32_t position);
    Transport::JackTransportState getTransportState();
    void enableTimebaseMaster();
    void disableTimebaseMaster();
    void deactivate();
    void startFreewheel();
    void stopFreewheel();
    void setBufferSize(uint32_t bufSize);
    uint32_t getSampleRate();

    std::unique_ptr<JackAudioInputPort> createAudioInputPort(const char* name);
    std::unique_ptr<JackAudioOutputPort> createAudioOutputPort(const char* name);
    std::unique_ptr<JackMIDIInputPort> createMIDIInputPort(const char* name);
    std::unique_ptr<JackMIDIOutputPort> createMIDIOutputPort(const char* name);

    std::vector<std::unique_ptr<JackAudioInputPort>> getAudioInputPorts();
    std::vector<std::unique_ptr<JackAudioOutputPort>> getAudioOutputPorts();
    std::vector<std::unique_ptr<JackMIDIInputPort>> getMIDIInputPorts();
    std::vector<std::unique_ptr<JackMIDIOutputPort>> getMIDIOutputPorts();
};
#endif
