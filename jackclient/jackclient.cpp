#include "jackclient.h"
jack_nframes_t JackClient::nframes = 0;

JackPort::JackPort(JackClient* client, jack_port_t* port, JackPortType type):client(client),portType(type){
	this->client_handle = client->client;
	this->port = port;
};
JackInputPort::JackInputPort(JackClient* client, jack_port_t* port, JackPortType type):JackPort(client,port,type){};
JackOutputPort::JackOutputPort(JackClient* client, jack_port_t* port, JackPortType type):JackPort(client,port,type){};
JackAudioInputPort::JackAudioInputPort(JackClient* client, jack_port_t* port):JackInputPort(client,port,JackPortType::Audio){};
JackAudioOutputPort::JackAudioOutputPort(JackClient* client, jack_port_t* port):JackOutputPort(client,port,JackPortType::Audio){};
JackMIDIInputPort::JackMIDIInputPort(JackClient* client, jack_port_t* port):JackInputPort(client,port,JackPortType::MIDI){};
JackMIDIOutputPort::JackMIDIOutputPort(JackClient* client, jack_port_t* port):JackOutputPort(client,port,JackPortType::MIDI){};

JackPort::JackPort(JackClient* client, const char* name, JackPortType type):client(client),portType(type){
	this->client_handle = client->client;
};
JackPort::~JackPort(){};
JackInputPort::JackInputPort(JackClient* client, const char* name, JackPortType type):JackPort(client,name,type){
	if(type == JackPortType::Audio)
		this->port =  jack_port_register (this->client_handle, name, JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput, 0);
	else if(type == JackPortType::MIDI)
		this->port =  jack_port_register (this->client_handle, name, JACK_DEFAULT_MIDI_TYPE, JackPortIsInput, 0);
};
JackOutputPort::JackOutputPort(JackClient* client, const char* name, JackPortType type):JackPort(client,name,type){
	if(type == JackPortType::Audio)
		this->port =  jack_port_register (this->client_handle, name, JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);
	else if(type == JackPortType::MIDI)
		this->port =  jack_port_register (this->client_handle, name, JACK_DEFAULT_MIDI_TYPE, JackPortIsOutput, 0);
};
JackAudioInputPort::JackAudioInputPort(JackClient* client, const char* name):JackInputPort(client,name,JackPortType::Audio){};
JackAudioOutputPort::JackAudioOutputPort(JackClient* client, const char* name):JackOutputPort(client,name,JackPortType::Audio){};
JackMIDIInputPort::JackMIDIInputPort(JackClient* client, const char* name):JackInputPort(client,name,JackPortType::MIDI){};
JackMIDIOutputPort::JackMIDIOutputPort(JackClient* client, const char* name):JackOutputPort(client,name,JackPortType::MIDI){};
JackMIDIEvent::JackMIDIEvent(uint32_t time, size_t size, unsigned char* buffer):time(time),size(size),buffer(buffer){};
uint32_t JackMIDIEvent::getTime(){return this->time;};
size_t JackMIDIEvent::getSize(){return this->size;};
unsigned char* JackMIDIEvent::getMIDIData(){return this->buffer;};

void* JackPort::getBufferInternal()
{
	return jack_port_get_buffer (this->port, JackClient::nframes);
}

std::string JackPort::getName()
{
	return std::string(jack_port_name(this->port));
}

std::string JackPort::getShortName()
{
	return std::string(jack_port_short_name(this->port));
}

bool JackPort::isPhysical()
{
	return ((jack_port_flags(this->port) & JackPortIsPhysical) == JackPortIsPhysical);
}

bool JackPort::isMine()
{
	return jack_port_is_mine(this->client_handle, this->port)?true:false;
}

JackPortType JackPort::getPortType()
{
	return this->portType;
}

void JackPort::disconnectAll()
{
    jack_port_disconnect(this->client_handle,this->port);
}

float* JackAudioInputPort::getBuffer()
{
	return (float*)getBufferInternal();
}

float* JackAudioOutputPort::getBuffer()
{
	return (float*)getBufferInternal();
}

void JackOutputPort::connectTo(JackInputPort* inPort)
{
	if(this->client->getState() != JackState::ACTIVE)
		throw JackClientException("Cannot connect ports when client is not active");
	int err = jack_connect(this->client_handle, jack_port_name(this->port),jack_port_name(inPort->port));
	if(err != 0 && err != EEXIST)
		throw JackClientException("Connecting port failed");
}

void JackOutputPort::disconnect(JackInputPort* inPort)
{
	jack_disconnect(this->client_handle, jack_port_name(this->port),jack_port_name(inPort->port));
}
void JackInputPort::connectTo(JackOutputPort* outPort)
{
	if(this->client->getState() != JackState::ACTIVE)
		throw JackClientException("Cannot connect ports when client is not active");
	int err = jack_connect(this->client_handle, jack_port_name(outPort->port), jack_port_name(this->port));
	if(err != 0 && err != EEXIST)
		throw JackClientException("Connecting port failed");
}

void JackInputPort::disconnect(JackOutputPort* outPort)
{
	jack_disconnect(this->client_handle, jack_port_name(outPort->port), jack_port_name(this->port));
}

std::vector<JackMIDIEvent*> JackMIDIInputPort::getMIDIEvents()
{
	std::vector<JackMIDIEvent*> events;
	void* internalBuf = this->getBufferInternal();
	uint32_t count = jack_midi_get_event_count(internalBuf);

	for(uint32_t i=0;i<count; i++)
	{
		jack_midi_event_t ev;
		if(!jack_midi_event_get(&ev,this->getBufferInternal(),i))
		{
			events.push_back(new JackMIDIEvent(ev.time,ev.size,ev.buffer));
		}
	}
	return events;
}
void JackMIDIOutputPort::clearBuffer()
{
	 jack_midi_clear_buffer(getBufferInternal());
}
void JackMIDIOutputPort::writeEvent(JackMIDIEvent& event)
{
	this->write(event.getTime(),event.getMIDIData(),event.getSize());
}

void JackMIDIOutputPort::write(uint32_t time, unsigned char* buffer, size_t size)
{
	//Exception ? Return ErrorCode ? Do Nothing if it fails ?
	jack_midi_event_write(getBufferInternal(),time,buffer,size);
}

JackMIDIInputPort::~JackMIDIInputPort()
{

}

JackClient::JackClient(const char* name): name(name){}
JackClient::~JackClient()
{}
JackState JackClient::getState()
{
	return jackState;
}
int JackClient::process(jack_nframes_t nframes, void *arg)
{
    JackClient* cl = static_cast<JackClient*>(arg);
    return cl->onProcess(nframes);
}

void JackClient::jack_shutdown(void * arg){
    static_cast<JackClient*>(arg)->jackState = JackState::CLOSED;
	return static_cast<JackClient*>(arg)->onShutdown();
};

int JackClient::buffer_size_callback(jack_nframes_t nframes, void *arg)
{
	JackClient::nframes = nframes;
	return 0;
}

int JackClient::xrun_callback (void *arg)
{
    static_cast<JackClient*>(arg)->onXRun();
    return 0;
}

void JackClient::open()
{
	this->client = jack_client_open (this->name, JackNoStartServer, &this->status, NULL);
    if (this->client == NULL) {
      if (this->status & JackServerFailed) {
	throw JackClientException("Unable to connect to JACK server");
      }
      return;
    }
  if (this->status & JackNameNotUnique)
	  this->name = jack_get_client_name(client);
  jack_set_process_callback (this->client, &JackClient::process, this);
  jack_on_shutdown (this->client, &JackClient::jack_shutdown, this);
  jack_set_buffer_size_callback	(this->client,&JackClient::buffer_size_callback,this);
  jack_set_xrun_callback (this->client, &JackClient::xrun_callback, this);
  jackState = JackState::INACTIVE;
};
void JackClient::close()
{
    if(jackState != JackState::CLOSED)
    {
        if(this->client != NULL)
            jack_client_close(this->client) ;
        jackState = JackState::CLOSED;
    }
};
void JackClient::activate()
{
	if (( jackState == JackState::INACTIVE ) && this->client != NULL && jack_activate (this->client)) {
      throw JackClientException("cannot activate client");
  }
  jackState = JackState::ACTIVE;
};
void JackClient::deactivate()
{
    if(jackState == JackState::ACTIVE)
        jack_deactivate (client);
	jackState = JackState::INACTIVE;
};

JackInputPort* JackClient::createInputPort(const char* name, JackPortType type)
{
    if(jackState == JackState::CLOSED)
        throw JackClientException("cannot create port when client is not opened");
	switch(type)
	{
		case(JackPortType::Audio):
		{
			return new JackAudioInputPort(this,name);
		}
		case(JackPortType::MIDI):
		{
			return new JackMIDIInputPort(this,name);
		}
	}
	throw JackClientException("Hu! This should never happen. JackPortType is unknown");
}
JackOutputPort* JackClient::createOutputPort(const char* name, JackPortType type)
{
    if(jackState == JackState::CLOSED)
        throw JackClientException("cannot create port when client is not opened");
    switch(type)
	{
		case(JackPortType::Audio):
		{
			return new JackAudioOutputPort(this,name);
		}
		case(JackPortType::MIDI):
		{
			return new JackMIDIOutputPort(this,name);
		}
	}
	throw JackClientException("Hu! This should never happen. JackPortType is unknown");
}

void JackClient::startFreewheel()
{
	if( jack_set_freewheel(this->client, 1) )
		throw JackClientException("Error starting freewheel-mode");
}

void JackClient::stopFreewheel()
{
	if( jack_set_freewheel(this->client, 0) )
		throw JackClientException("Error stopping freewheel-mode");
}

const char ** JackClient::listPorts(JackPortType filter, bool onlyPhysical, bool listInputs)
{
	long _filter = onlyPhysical?JackPortIsPhysical:0;
	if(listInputs)
		_filter|=JackPortIsInput;
	else
		_filter|=JackPortIsOutput;
	if(filter == JackPortType::MIDI)
		return jack_get_ports (this->client, NULL, JACK_DEFAULT_MIDI_TYPE, _filter);
	else
		return jack_get_ports (this->client, NULL, JACK_DEFAULT_AUDIO_TYPE, _filter);
}


std::vector<JackInputPort*> JackClient::listPhysicalInputPorts(JackPortType filter)
{
	const char ** portList = this->listPorts(filter, true, true);
	return getInputPorts(filter,portList);
}

std::vector<JackInputPort*> JackClient::listInputPorts(JackPortType filter)
{
	const char ** portList = this->listPorts(filter, false, true);
	return getInputPorts(filter,portList);
}

std::vector<JackInputPort*> JackClient::getInputPorts(JackPortType filter,const char ** portList)
{
    std::vector<JackInputPort*> list;
    if(portList)
	{
		unsigned int portNum = 0;
		while(portList[portNum])
		{
			jack_port_t* _port = jack_port_by_name(this->client, portList[portNum]);
			JackInputPort* inPort;
			if(filter == JackPortType::Audio)
				inPort = new JackAudioInputPort(this,_port);
			 else
				inPort = new JackMIDIInputPort(this,_port);
			list.push_back(inPort);
			portNum++;
		}
		jack_free(portList);
	}
	return list;
};
std::vector<JackOutputPort*> JackClient::getOutputPorts(JackPortType filter, const char ** portList)
{
    std::vector<JackOutputPort*> list;
    if(portList)
	{
		unsigned int portNum = 0;
		while(portList[portNum])
		{
			jack_port_t* _port = jack_port_by_name(this->client, portList[portNum]);
			JackOutputPort* outPort;
			if(filter == JackPortType::Audio)
				outPort = new JackAudioOutputPort(this,_port);
            else
				outPort = new JackMIDIOutputPort(this,_port);
			list.push_back(outPort);
			portNum++;
		}
		jack_free(portList);
	}
	return list;
};

std::vector<JackOutputPort*> JackClient::listPhysicalOutputPorts(JackPortType filter)
{
	const char ** portList = this->listPorts(filter, true, false);
	return getOutputPorts(filter,portList);
}

std::vector<JackOutputPort*> JackClient::listOutputPorts(JackPortType filter)
{
	const char ** portList = this->listPorts(filter, false, false);
	return getOutputPorts(filter,portList);
}

uint32_t JackClient::getSampleRate()
{
	return jack_get_sample_rate(this->client);
}
