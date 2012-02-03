    			     Viva Voce 1.3
		      RTP Voice Chat Application
			 November 2003 (Updated 2011 - Feb 2012)

		      Developed by Kunal Kandekar

* INTRODUCTION
This program implements a voice chat program over UDP. It  uses  ADPCM  for
audio compression. Voice capture and playback is done through the soundcard  
of the local machine. The implementation is highly multithreaded to improve 
interactivity of the application.
 
SIP is used for signalling, and RTP is the multimedia transport protocol.

Almost all of this was written in 2003 for a graduate  course  project over 
the course of a semester. As was  required by the  instructor, this project
was  developed alone, from scratch, based on reading and  understanding  of 
the corresponding RFCs. RFC  compliance was tested by making sure each sub-
mitted project would interoperate properly with any  other  student's  sub-
mission.

* MAKE
Unzip the vivavoce.zip file and run make in <install-dir> The follow-
ing core files should be present:
	VivaVoce.h, VivaVoce.cpp - the main program file and user interface
	UserAgent.cpp            - Thread for SIP management
	AdpcmCodec.cpp           - Thread that invokes the ADPCM codec		
	AudioRW.cpp              - Threads to read/write audio
	UdpRW.cpp                - Threads to read/write data over UDP
	ResourceManager.cpp      - Centralized resource manager functions	
	RTPManager.cpp           - RTP management functions
	RTP.h, RTP.cpp		     - RTP Related functionality	
	SDP.h, SDP.cpp		     - SDP Related functionality	
	SIP.h, SIP.cpp		     - SIP Related functionality	
    OSXCoreAudioIO.h/.cpp    - OS X Audio related functionality

Incuded library files:
	kodek.h, kodek.cpp          - ADPCM codec implementation
    ulaw.h, ulaw.c              - uLaw audio codec functionality
	Thread.h, Thread.cpp        - C++ Thread/Events class package
	SyncQueue.h, SyncQueue.cpp  - Thread-safe asynchronous queue 

Platform: Originally for Solaris, but hasn't been tested on it since changes. 
Currently has only been compiled and tested on Mac OS X (10.6) and Linux 
(Ubuntu 10.04)

* RUN
The program can be run as follows:
Step 1) make 
Step 2) vivavoce <user-name>
Step 3) Start chatting!
Step 4) Commands can be entered on the command line:
	a) 'invite user@host' - Invite the name user at a host
	b) 'accept' or 'a'  - accept an incoming call.
	c) 'reject' or 'r'  - reject an incoming call (Decline).
	d) 'bye' or 'b'     - terminate the current in-progress call.
	e) 'exit' or 'x'    - quits program
	f) 'diag'           - displays diagnostics
	g) 'verbose' or 'v' - verbose mode: displays all SIP and other data
	h) 'test' or 't'    - test audio:  replays recorded audio delayed by 2s
	i) 'mute' or 'm'    - toggles mute audio.
	j) 'sine' or 's'    - replaces audio in with a constant sine wave tone.
The network statistics are collected in a file named stats<SSRC>.csv.  This 
file can be opened in excel to facilitate plotting of graphs.

* Jitter, Loss and RTT
Jitter and Loss are not simulated here. The RTT is calculated as  specified
in RFC 3550. Note that the initial values of jitter are incorrect since  it
assumes the earlier jitter to be 0.

* Audio Compression
IMA ADPCM compression is applied to the signed PCM samples derived from the
recorded u-Law data (recording and playback are done in u-Law format on the
sound card.

* Network Transport
Transport is over UDP sockets. The header formats used follow  the  RTP and  
RTCP specifications (rfc3550).

* Signalling
The basic call flow has been implemented as per the  specifications.  Basic
reliability is provided in the  SIP  signalling as it is over UDP.  However, 
this reliability is not as complete as that specified in RFC 2543.

* Audio Quality
This program is full duplex. Audio quality  has improved considerably since
the previous version, and the playout latency has been eliminated as well. 


* KNOWN BUGS
Tons... 
Many bugs are related to concurrency issues. Shortness of time and  general
laziness have resultedin the complete lack of proper mutex usage around SIP
signalling  operations.  E.g.  sometimes it is unable to accept a call even 
if it is ringing and connectivity is fine.
