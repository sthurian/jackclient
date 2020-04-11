#include "jackclient.h"
jack_nframes_t JackClient::nframes = 0;

JackPort::JackPort(JackClient* client, jack_port_t* port, JackPortType type)
    : client(client), portType(type) {
    this->client_handle = client->client;
    this->port = port;
}

JackInputPort::JackInputPort(JackClient* client, jack_port_t* port, JackPortType type)
    : JackPort(client, port, type) {}
JackOutputPort::JackOutputPort(JackClient* client, jack_port_t* port, JackPortType type)
    : JackPort(client, port, type) {}
JackAudioInputPort::JackAudioInputPort(JackClient* client, jack_port_t* port)
    : JackInputPort(client, port, JackPortType::AUDIO) {}
JackAudioOutputPort::JackAudioOutputPort(JackClient* client, jack_port_t* port)
    : JackOutputPort(client, port, JackPortType::AUDIO) {}
JackMIDIInputPort::JackMIDIInputPort(JackClient* client, jack_port_t* port)
    : JackInputPort(client, port, JackPortType::MIDI) {}
JackMIDIOutputPort::JackMIDIOutputPort(JackClient* client, jack_port_t* port)
    : JackOutputPort(client, port, JackPortType::MIDI) {}

JackPort::JackPort(JackClient* client, const char* name, JackPortType type)
    : client(client), portType(type) {
    this->client_handle = client->client;
}

JackPort::~JackPort() {}

JackInputPort::JackInputPort(JackClient* client, const char* name, JackPortType type)
    : JackPort(client, name, type) {
    if (type == JackPortType::AUDIO)
        this->port = jack_port_register(this->client_handle, name, JACK_DEFAULT_AUDIO_TYPE,
                                        JackPortIsInput, 0);
    else if (type == JackPortType::MIDI)
        this->port = jack_port_register(this->client_handle, name, JACK_DEFAULT_MIDI_TYPE,
                                        JackPortIsInput, 0);
}

JackOutputPort::JackOutputPort(JackClient* client, const char* name, JackPortType type)
    : JackPort(client, name, type) {
    if (type == JackPortType::AUDIO)
        this->port = jack_port_register(this->client_handle, name, JACK_DEFAULT_AUDIO_TYPE,
                                        JackPortIsOutput, 0);
    else if (type == JackPortType::MIDI)
        this->port = jack_port_register(this->client_handle, name, JACK_DEFAULT_MIDI_TYPE,
                                        JackPortIsOutput, 0);
}

JackAudioInputPort::JackAudioInputPort(JackClient* client, const char* name)
    : JackInputPort(client, name, JackPortType::AUDIO) {}
JackAudioOutputPort::JackAudioOutputPort(JackClient* client, const char* name)
    : JackOutputPort(client, name, JackPortType::AUDIO) {}

JackMIDIInputPort::JackMIDIInputPort(JackClient* client, const char* name)
    : JackInputPort(client, name, JackPortType::MIDI) {}
JackMIDIOutputPort::JackMIDIOutputPort(JackClient* client, const char* name)
    : JackOutputPort(client, name, JackPortType::MIDI) {}

JackMIDIEvent::JackMIDIEvent(uint32_t time, size_t size, unsigned char* buffer)
    : time(time), size(size), buffer(buffer) {}

uint32_t JackMIDIEvent::getTime() { return this->time; }
size_t JackMIDIEvent::getSize() { return this->size; }

unsigned char* JackMIDIEvent::getMIDIData() { return this->buffer; }

void* JackPort::getBufferInternal() {
    return jack_port_get_buffer(this->port, JackClient::nframes);
}

template <typename T>
std::vector<std::unique_ptr<T>> JackPort::listConnections() {
    std::vector<std::unique_ptr<T>> list;
    const char** portList = jack_port_get_all_connections(this->client_handle, this->port);
    if (portList) {
        unsigned int portNum = 0;
        while (portList[portNum]) {
            jack_port_t* _port = jack_port_by_name(this->client_handle, portList[portNum]);
            list.push_back(std::make_unique<T>(this->client, _port));
            portNum++;
        }
        jack_free(portList);
    }
    return list;
}

std::string JackPort::getName() { return std::string(jack_port_name(this->port)); }
std::string JackPort::getShortName() { return std::string(jack_port_short_name(this->port)); }

bool JackPort::isPhysical() {
    return ((jack_port_flags(this->port) & JackPortIsPhysical) == JackPortIsPhysical);
}

bool JackPort::isMine() {
    return jack_port_is_mine(this->client_handle, this->port) ? true : false;
}

JackPortType JackPort::getPortType() { return this->portType; }

void JackPort::disconnectAll() { jack_port_disconnect(this->client_handle, this->port); }

float* JackAudioInputPort::getBuffer() { return (float*)getBufferInternal(); }

float* JackAudioOutputPort::getBuffer() { return (float*)getBufferInternal(); }

void JackOutputPort::connectTo(JackInputPort& inPort) {
    if (this->getPortType() != inPort.getPortType())
        throw JackClientException("Cannot connect ports with different types");
    if (this->client->getState() != JackState::ACTIVE)
        throw JackClientException("Cannot connect ports when client is not active");
    int err =
        jack_connect(this->client_handle, jack_port_name(this->port), jack_port_name(inPort.port));
    if (err != 0 && err != EEXIST) throw JackClientException("Connecting port failed");
}

void JackOutputPort::disconnect(JackInputPort& inPort) {
    jack_disconnect(this->client_handle, jack_port_name(this->port), jack_port_name(inPort.port));
}

void JackInputPort::connectTo(JackOutputPort& outPort) {
    if (this->getPortType() != outPort.getPortType())
        throw JackClientException("Cannot connect ports with different types");
    if (this->client->getState() != JackState::ACTIVE)
        throw JackClientException("Cannot connect ports when client is not active");
    int err =
        jack_connect(this->client_handle, jack_port_name(outPort.port), jack_port_name(this->port));
    if (err != 0 && err != EEXIST) throw JackClientException("Connecting port failed");
}

void JackInputPort::disconnect(JackOutputPort& outPort) {
    jack_disconnect(this->client_handle, jack_port_name(outPort.port), jack_port_name(this->port));
}

std::vector<std::unique_ptr<JackAudioOutputPort>> JackAudioInputPort::getConnections() {
    return this->listConnections<JackAudioOutputPort>();
}

std::vector<std::unique_ptr<JackAudioInputPort>> JackAudioOutputPort::getConnections() {
    return this->listConnections<JackAudioInputPort>();
}

std::vector<std::unique_ptr<JackMIDIEvent>> JackMIDIInputPort::getMIDIEvents() {
    std::vector<std::unique_ptr<JackMIDIEvent>> events;
    void* internalBuf = this->getBufferInternal();
    uint32_t count = jack_midi_get_event_count(internalBuf);

    for (uint32_t i = 0; i < count; i++) {
        jack_midi_event_t ev;
        if (!jack_midi_event_get(&ev, this->getBufferInternal(), i)) {
            events.push_back(std::make_unique<JackMIDIEvent>(ev.time, ev.size, ev.buffer));
        }
    }
    return events;
}

void JackMIDIOutputPort::clearBuffer() { jack_midi_clear_buffer(getBufferInternal()); }

void JackMIDIOutputPort::writeEvent(JackMIDIEvent& event) {
    this->write(event.getTime(), event.getMIDIData(), event.getSize());
}

void JackMIDIOutputPort::write(uint32_t time, unsigned char* buffer, size_t size) {
    // Exception ? Return ErrorCode ? Do Nothing if it fails?
    jack_midi_event_write(getBufferInternal(), time, buffer, size);
}

JackMIDIInputPort::~JackMIDIInputPort() {}

JackClient::JackClient(const char* name) : name(name) {}
JackClient::~JackClient() {}

JackState JackClient::getState() { return jackState; }

int JackClient::process(jack_nframes_t nframes, void* arg) {
    JackClient* cl = static_cast<JackClient*>(arg);
    return cl->onProcess(nframes);
}

void JackClient::jack_shutdown(void* arg) {
    static_cast<JackClient*>(arg)->jackState = JackState::CLOSED;
    return static_cast<JackClient*>(arg)->onShutdown();
}

int JackClient::buffer_size_callback(jack_nframes_t nframes, void* arg) {
    JackClient::nframes = nframes;
    return 0;
}

int JackClient::xrun_callback(void* arg) {
    static_cast<JackClient*>(arg)->onXRun();
    return 0;
}

int JackClient::sync_callback(jack_transport_state_t state, jack_position_t* pos, void* arg) {
    return static_cast<JackClient*>(arg)->onTransportSync(
        static_cast<Transport::JackTransportState>(state),
        static_cast<Transport::JackPosition*>(pos));
}

void JackClient::timebase_callback(jack_transport_state_t state, jack_nframes_t nframes,
                                   jack_position_t* pos, int new_pos, void* arg) {
    static_cast<JackClient*>(arg)->onTimebase(static_cast<Transport::JackTransportState>(state),
                                              nframes, static_cast<Transport::JackPosition*>(pos),
                                              new_pos ? true : false);
}

void JackClient::open() {
    this->client = jack_client_open(this->name, JackNoStartServer, &this->status, NULL);
    if (this->client == NULL) {
        if (this->status & JackServerFailed) {
            throw JackClientException("Unable to connect to JACK server");
        }
        return;
    }
    if (this->status & JackNameNotUnique) this->name = jack_get_client_name(client);
    jack_set_process_callback(this->client, &JackClient::process, this);
    jack_on_shutdown(this->client, &JackClient::jack_shutdown, this);
    jack_set_buffer_size_callback(this->client, &JackClient::buffer_size_callback, this);
    jack_set_xrun_callback(this->client, &JackClient::xrun_callback, this);
    jack_set_sync_callback(this->client, &JackClient::sync_callback, this);
    jackState = JackState::INACTIVE;
}

void JackClient::close() {
    if (jackState != JackState::CLOSED) {
        if (this->client != NULL) jack_client_close(this->client);
        jackState = JackState::CLOSED;
    }
}

void JackClient::activate() {
    if ((jackState == JackState::INACTIVE) && this->client != NULL && jack_activate(this->client)) {
        throw JackClientException("cannot activate client");
    }
    jackState = JackState::ACTIVE;
}

void JackClient::deactivate() {
    if (jackState == JackState::ACTIVE) jack_deactivate(client);
    jackState = JackState::INACTIVE;
}

std::unique_ptr<JackAudioInputPort> JackClient::createAudioInputPort(const char* name) {
    if (jackState == JackState::CLOSED)
        throw JackClientException("cannot create port when client is not opened");
    return std::make_unique<JackAudioInputPort>(this, name);
}

std::unique_ptr<JackAudioOutputPort> JackClient::createAudioOutputPort(const char* name) {
    if (jackState == JackState::CLOSED)
        throw JackClientException("cannot create port when client is not opened");
    return std::make_unique<JackAudioOutputPort>(this, name);
}

std::unique_ptr<JackMIDIInputPort> JackClient::createMIDIInputPort(const char* name) {
    if (jackState == JackState::CLOSED)
        throw JackClientException("cannot create port when client is not opened");
    return std::make_unique<JackMIDIInputPort>(this, name);
}

std::unique_ptr<JackMIDIOutputPort> JackClient::createMIDIOutputPort(const char* name) {
    if (jackState == JackState::CLOSED)
        throw JackClientException("cannot create port when client is not opened");
    return std::make_unique<JackMIDIOutputPort>(this, name);
}

void JackClient::startFreewheel() {
    if (jack_set_freewheel(this->client, 1))
        throw JackClientException("Error starting freewheel-mode");
}

void JackClient::stopFreewheel() {
    if (jack_set_freewheel(this->client, 0))
        throw JackClientException("Error stopping freewheel-mode");
}

std::vector<std::unique_ptr<JackAudioInputPort>> JackClient::getAudioInputPorts() {
    return this->createPorts<JackAudioInputPort>(JackPortType::AUDIO,
                                                 JackPortFlags::JackPortIsInput);
}

std::vector<std::unique_ptr<JackAudioOutputPort>> JackClient::getAudioOutputPorts() {
    return this->createPorts<JackAudioOutputPort>(JackPortType::AUDIO,
                                                  JackPortFlags::JackPortIsOutput);
}

std::vector<std::unique_ptr<JackMIDIInputPort>> JackClient::getMIDIInputPorts() {
    return this->createPorts<JackMIDIInputPort>(JackPortType::MIDI, JackPortFlags::JackPortIsInput);
}
std::vector<std::unique_ptr<JackMIDIOutputPort>> JackClient::getMIDIOutputPorts() {
    return this->createPorts<JackMIDIOutputPort>(JackPortType::MIDI,
                                                 JackPortFlags::JackPortIsOutput);
}

template <typename T>
std::vector<std::unique_ptr<T>> JackClient::createPorts(JackPortType type, JackPortFlags flags) {
    std::vector<std::unique_ptr<T>> list;
    const char** portList;

    if (type == JackPortType::AUDIO) {
        portList = jack_get_ports(this->client, NULL, JACK_DEFAULT_AUDIO_TYPE, flags);
    } else {
        portList = jack_get_ports(this->client, NULL, JACK_DEFAULT_MIDI_TYPE, flags);
    }

    unsigned int portNum = 0;
    while (portList[portNum]) {
        jack_port_t* _port = jack_port_by_name(this->client, portList[portNum]);
        list.push_back(std::make_unique<T>(this, _port));
        portNum++;
    }
    jack_free(portList);
    return list;
}

void JackClient::stopTransport() { jack_transport_stop(this->client); }

void JackClient::startTransport() { jack_transport_start(this->client); }

void JackClient::setTransportPosition(uint32_t position) {
    jack_transport_locate(this->client, position);
}

int JackClient::onTransportSync(Transport::JackTransportState state, Transport::JackPosition* pos) {
    switch (state) {
        case Transport::JackTransportState::STARTING:
            return this->onTransportStart(pos);
            break;
        case Transport::JackTransportState::STOPPED:
            return this->onTransportStop(pos);
            break;
        case Transport::JackTransportState::ROLLING:
            return this->onTransportRoll(pos);
            break;
        default:
            break;
    }
    return 0;
}

void JackClient::enableTimebaseMaster() {
    if (jack_set_timebase_callback(this->client, 0, &JackClient::timebase_callback, this))
        throw JackClientException("Could not enable Timebase Master");
}

void JackClient::disableTimebaseMaster() {
    if (jack_release_timebase(this->client))
        throw JackClientException("Could not disable Timebase Master");
}

Transport::JackTransportState JackClient::getTransportState() {
    return static_cast<Transport::JackTransportState>(jack_transport_query(this->client, NULL));
}

uint32_t JackClient::getSampleRate() { return jack_get_sample_rate(this->client); }