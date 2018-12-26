#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include "log.h"
#include "mp4recorder.h"


mp4track::mp4track(MP4FileHandle mp4)
{
	// Set struct info
	this->mp4 = mp4;
	track = 0;
	hint = 0;
	length = 0;
	sampleId = 0;
	first = 1;
	frame = NULL;
}

int mp4track::CreateAudioTrack(AudioCodec::Type codec,DWORD rate)
{
	Log("mp4track::CreateAudioTrack [codec:%d]\n",codec);
	
	BYTE type;

	//Check the codec
	switch (codec)
	{
		case AudioCodec::PCMU:
		{
			// Create audio track
			track = MP4AddULawAudioTrack(mp4,rate);
			// Create audio hint track
			hint = MP4AddHintTrack(mp4, track);
			// Set payload type for hint track
			type = 0;
			MP4SetHintTrackRtpPayload(mp4, hint, "PCMU", &type, 0, NULL, 1, 0);
			// Set channel and sample properties
			MP4SetTrackIntegerProperty(mp4, track, "mdia.minf.stbl.stsd.ulaw.channels", 1);
			MP4SetTrackIntegerProperty(mp4, track, "mdia.minf.stbl.stsd.ulaw.sampleSize", 8);
			break;
		}
		case AudioCodec::PCMA:
		{
			// Create audio track
			track = MP4AddALawAudioTrack(mp4,rate);
			// Set channel and sample properties
			MP4SetTrackIntegerProperty(mp4, track, "mdia.minf.stbl.stsd.alaw.channels", 1);
			MP4SetTrackIntegerProperty(mp4, track, "mdia.minf.stbl.stsd.alaw.sampleSize", 8);
			// Create audio hint track
			hint = MP4AddHintTrack(mp4, track);
			// Set payload type for hint track
			type = 8;
			MP4SetHintTrackRtpPayload(mp4, hint, "PCMA", &type, 0, NULL, 1, 0);
			break;
		}
		case AudioCodec::OPUS:
		{
#ifdef MP4_OPUS_AUDIO_TYPE
			// Create audio track
			track = MP4AddOpusAudioTrack(mp4, rate, 2, 640);
#else      
			// Create audio track
			track = MP4AddAudioTrack(mp4, rate, 1024, MP4_PRIVATE_AUDIO_TYPE);
#endif       
			// Create audio hint track
			hint = MP4AddHintTrack(mp4, track);
			// Set payload type for hint track
			type = 102;
			MP4SetHintTrackRtpPayload(mp4, hint, "OPUS", &type, 0, NULL, 1, 0);
			break;
		}
		default:
			return 0;
	}

	return track;
}

int mp4track::CreateVideoTrack(VideoCodec::Type codec,int width, int height)
{
	
	Log("mp4track::CreateVideoTrack [codec:%d]\n",codec);
	
	BYTE type;

	//Check the codec
	switch (codec)
	{
		case VideoCodec::H263_1996:
		{
			// Create video track
			track = MP4AddH263VideoTrack(mp4, 90000, 0, width, height, 0, 0, 0, 0);
			// Create video hint track
			hint = MP4AddHintTrack(mp4, track);
			// Set payload type for hint track
			type = 34;
			MP4SetHintTrackRtpPayload(mp4, hint, "H263", &type, 0, NULL, 1, 0);
			break;
		}
		case VideoCodec::H263_1998:
		{
			// Create video track
			track = MP4AddH263VideoTrack(mp4, 90000, 0, width, height, 0, 0, 0, 0);
			// Create video hint track
			hint = MP4AddHintTrack(mp4, track);
			// Set payload type for hint track
			type = 96;
			MP4SetHintTrackRtpPayload(mp4, hint, "H263-1998", &type, 0, NULL, 1, 0);
			break;
		}
		case VideoCodec::H264:
		{
			// Should parse video packet to get this values
			unsigned char AVCProfileIndication 	= 0x42;	//Baseline
			unsigned char AVCLevelIndication	= 0x0D;	//1.3
			unsigned char AVCProfileCompat		= 0xC0;
			MP4Duration h264FrameDuration		= 1.0/30;
			// Create video track
			track = MP4AddH264VideoTrack(mp4, 90000, h264FrameDuration, width, height, AVCProfileIndication, AVCProfileCompat, AVCLevelIndication,  3);
			// Create video hint track
			hint = MP4AddHintTrack(mp4, track);
			// Set payload type for hint track
			type = 99;
			MP4SetHintTrackRtpPayload(mp4, hint, "H264", &type, 0, NULL, 1, 0);
			break;
		}
		case VideoCodec::VP8:
		{
			// Should parse video packet to get this values
			MP4Duration hvp8FrameDuration	= 1.0/30;
#ifdef MP4_VP8_VIDEO_TYPE      
			// Create video track
			track = MP4AddVP8VideoTrack(mp4, 90000, hvp8FrameDuration, width, height);
#else
			// Create video track
			track = MP4AddVideoTrack(mp4, 90000, hvp8FrameDuration, width, height, MP4_PRIVATE_VIDEO_TYPE);
#endif
			// Create video hint track
			hint = MP4AddHintTrack(mp4, track);
			// Set payload type for hint track
			type = 101;
			MP4SetHintTrackRtpPayload(mp4, hint, "VP8", &type, 0, NULL, 1, 0);
			break;
		}
		case VideoCodec::VP9:
		{
			// Should parse video packet to get this values
			MP4Duration vp9FrameDuration	= 1.0/30;
#ifdef MP4_VP9_VIDEO_TYPE      
			// Create video track
			track = MP4AddVP9VideoTrack(mp4, 90000, vp9FrameDuration, width, height);
#else
			// Create video track
			track = MP4AddVideoTrack(mp4, 90000, vp9FrameDuration, width, height, MP4_PRIVATE_VIDEO_TYPE);
#endif
			// Create video hint track
			hint = MP4AddHintTrack(mp4, track);
			// Set payload type for hint track
			type = 102;
			MP4SetHintTrackRtpPayload(mp4, hint, "VP9", &type, 0, NULL, 1, 0);
			break;
		}		
		default:
			return Error("-Codec %s not supported yet\n",VideoCodec::GetNameFor(codec));
	}
	//OK
	return 1;
}

int mp4track::FlushAudioFrame(AudioFrame* frame,DWORD duration)
{
	// Save audio frame
	MP4WriteSample(mp4, track, frame->GetData(), frame->GetLength(), duration, 0, 1);

	//If as rtp info
	if (hint)
	{
		// Add rtp hint
		MP4AddRtpHint(mp4, hint);

		///Create packet
		MP4AddRtpPacket(mp4, hint, 0, 0);

		// Set full frame as data
		MP4AddRtpSampleData(mp4, hint, sampleId, 0, frame->GetLength());

		// Write rtp hint
		MP4WriteRtpHint(mp4, hint, duration, 1);
	}

	// Delete old one
	delete frame;
	//Stored
	return 1;
}

int mp4track::WriteAudioFrame(AudioFrame &audioFrame)
{
	//Store old one
	AudioFrame* prev = (AudioFrame*)frame;

	//Clone new one and store
	frame = audioFrame.Clone();

	//Check if we had and old frame
	if (!prev)
		//Exit
		return 0;
	
	//One more frame
	sampleId++;

	//Get number of samples
	DWORD duration = (frame->GetTimeStamp()-prev->GetTimeStamp())*audioFrame.GetRate()/1000;

	//Flush sample
	FlushAudioFrame((AudioFrame *)prev,duration);
	
	//Exit
	return 1;
}

int mp4track::FlushVideoFrame(VideoFrame* frame,DWORD duration)
{
	// Save video frame
	MP4WriteSample(mp4, track, frame->GetData(), frame->GetLength(), duration, 0, frame->IsIntra());

	//Check if we have rtp data
	if (frame->HasRtpPacketizationInfo())
	{
		//Get list
		const MediaFrame::RtpPacketizationInfo& rtpInfo = frame->GetRtpPacketizationInfo();
		//Add hint for frame
		MP4AddRtpHint(mp4, hint);
		//Get iterator
		MediaFrame::RtpPacketizationInfo::const_iterator it = rtpInfo.begin();
		//Latest?
		bool last = (it==rtpInfo.end());

		//Iterate
		while(!last)
		{
			//Get rtp packet and move to next
			MediaFrame::RtpPacketization *rtp = *(it++);
			//is last?
			last = (it==rtpInfo.end());

			//Create rtp packet
			MP4AddRtpPacket(mp4, hint, last, 0);

			//Prefix data can't be longer than 14bytes per mp4 spec
			if (rtp->GetPrefixLen() && rtp->GetPrefixLen()<14)
				//Add rtp data
				MP4AddRtpImmediateData(mp4, hint, rtp->GetPrefixData(), rtp->GetPrefixLen());

			//Add rtp data
			MP4AddRtpSampleData(mp4, hint, sampleId, rtp->GetPos(), rtp->GetSize());

		}
		//Save rtp
		MP4WriteRtpHint(mp4, hint, duration, frame->IsIntra());
	}

	// Delete old one
	delete frame;
	//Stored
	return 1;
}

int mp4track::WriteVideoFrame(VideoFrame& videoFrame)
{
	//Store old one
	VideoFrame* prev = (VideoFrame*)frame;

	//Clone new one and store
	frame = videoFrame.Clone();

	//Check if we had and old frame
	if (!prev)
		//Exit
		return 0;

	//One mor frame
	sampleId++;

	//Get samples
	DWORD duration = (frame->GetTimeStamp()-prev->GetTimeStamp())*90;

	//Flush frame
	FlushVideoFrame(prev,duration);
	
	//Not writed
	return 1;
}


int mp4track::Close()
{
	//If we got frame
	if (frame)
	{
		//If we have pre frame
		switch (frame->GetType())
		{
			case MediaFrame::Audio:
				//Flush it
				FlushAudioFrame((AudioFrame*)frame,8000);
				break;
			case MediaFrame::Video:
				//Flush it
				FlushVideoFrame((VideoFrame*)frame,90000);
				break;
			case MediaFrame::Unknown:
				//Nothing
				break;
		}
		//NO frame
		frame = NULL;
	}

	return 1;
}

MP4Recorder::MP4Recorder()
{
	recording = false;
	mp4 = MP4_INVALID_FILE_HANDLE;
	//No
	first = (QWORD)-1;
	//Create mutex
	pthread_mutex_init(&mutex,0);
}

MP4Recorder::~MP4Recorder()
{
        //Close just in case
        Close();
        
	//For each audio track
	for (Tracks::iterator it = audioTracks.begin(); it!=audioTracks.end(); ++it)
		//delete it
		delete(it->second);
	//For each video track
	for (Tracks::iterator it = videoTracks.begin(); it!=videoTracks.end(); ++it)
		//delete it
		delete(it->second);
	//Liberamos los mutex
	pthread_mutex_destroy(&mutex);
}

bool MP4Recorder::Create(const char* filename)
{
	Log("-MP4Recorder::Create() Opening mp4 recording [%s]\n",filename);

	//If we are recording
	if (mp4!=MP4_INVALID_FILE_HANDLE)
		//Close
		Close();

	// We have to wait for first I-Frame
	waitVideo = 0;

	// Create mp4 file
	mp4 = MP4Create(filename,0);

	// If failed
	if (mp4 == MP4_INVALID_FILE_HANDLE)
                //Error
		return Error("-Error opening mp4 file for recording\n");

	//Success
	return true;
}

bool MP4Recorder::Record()
{
	return Record(true);
}

bool MP4Recorder::Record(bool waitVideo)
{
        //Check mp4 file is opened
        if (mp4 == MP4_INVALID_FILE_HANDLE)
                //Error
                return Error("No MP4 file opened for recording\n");
        
	//Do We have to wait for first I-Frame?
	this->waitVideo = waitVideo;
	
	//Recording
	recording = true;
	
	//Exit
	return recording;
}

bool MP4Recorder::Stop()
{
	Log("-MP4Recorder::Stop()\n");
	
	//L0ck the  access to the file
	pthread_mutex_lock(&mutex);
	
        //not recording anymore
	recording = false;
	
	//L0ck the  access to the file
	pthread_mutex_unlock(&mutex);
	
	return true;
}

void* mp4close(void *mp4)
{
	timeval tv;
	getUpdDifTime(&tv);
	Log(">mp4close [%p]\n",mp4);
	// Close file
	MP4Close(mp4);
	Log("<mp4close [%p,time:%llu]\n",mp4,getDifTime(&tv)/1000);
	return NULL;
}
bool MP4Recorder::Close()
{
	//Default is async
	return Close(true);
}

bool MP4Recorder::Close(bool async)
{
        //Stop always
        Stop();
	
	//L0ck the  access to the file
	pthread_mutex_lock(&mutex);

	//Check mp4 file is opened
        if (mp4!=MP4_INVALID_FILE_HANDLE)
	{
		//For each audio track
		for (Tracks::iterator it = audioTracks.begin(); it!=audioTracks.end(); ++it)
			//Close it
			it->second->Close();
		//For each video track
		for (Tracks::iterator it = videoTracks.begin(); it!=videoTracks.end(); ++it)
			//Close it
			it->second->Close();

		
		//Do we need to launch the close on another thread?
		if (async) {
			//Launch MP4Close in another thread
			pthread_t 	mp4CloseThread;
			createPriorityThread(&mp4CloseThread,mp4close,mp4,0);
		} else {
			//Do it here
			mp4close(mp4);
		}

		//Empty file
		mp4 = MP4_INVALID_FILE_HANDLE;
	}
	
	//L0ck the  access to the file
	pthread_mutex_unlock(&mutex);
	
	//NOthing more
	return true;
}

void MP4Recorder::onMediaFrame(MediaFrame &frame)
{
	onMediaFrame(0,frame);
}

void MP4Recorder::onMediaFrame(DWORD ssrc, MediaFrame &frame)
{
	//Set now as timestamp
	onMediaFrame(ssrc,frame,getTimeMS());
}
void MP4Recorder::onMediaFrame(DWORD ssrc, MediaFrame &frame, QWORD time)
{
	// Check if we have to wait for video
	if (waitVideo && (frame.GetType()!=MediaFrame::Video))
		//Do nothing yet
		return;

	//L0ck the  access to the file
	pthread_mutex_lock(&mutex);
	
	//Check we are recording
	if (recording)
	{
		//Depending on the codec type
		switch (frame.GetType())
		{
			case MediaFrame::Audio:
			{
				//It is an audio track
				mp4track* audioTrack = NULL;
				//Find the ssrc
				Tracks::iterator it = audioTracks.find(ssrc);
				//If found
				if (it!=audioTracks.end())
					//Get it
					audioTrack = it->second;
				//Convert to audio frame
				AudioFrame &audioFrame = (AudioFrame&) frame;
				//Check if it is the first
				if (first==(QWORD)-1)
					//Set this one as first
					first = time;
				// Calculate new timestamp
				QWORD timestamp = time-first;
				// Check if we have the audio track
				if (!audioTrack)
				{
					//Create object
					audioTrack = new mp4track(mp4);
					//Create track
					audioTrack->CreateAudioTrack(audioFrame.GetCodec(),audioFrame.GetRate());
					//If it is not first
					if (timestamp)
					{
						//Create empty audio frame
						AudioFrame empty(audioFrame.GetCodec(),audioFrame.GetRate());
						//Set empty data
						empty.SetTimestamp(0);
						empty.SetLength(0);
						//Set duration until first real frame
						empty.SetDuration(timestamp);
						//Send first empty packet
						audioTrack->WriteAudioFrame(empty);
					}
					//Add it to map
					audioTracks[ssrc] = audioTrack;
				}

				//Update timestamp
				audioFrame.SetTimestamp(timestamp);
				// Save audio rtp packet
				audioTrack->WriteAudioFrame(audioFrame);
				break;
			}
			case MediaFrame::Video:
			{
				//It is an video track
				mp4track* videoTrack = NULL;
				//Find the ssrc
				Tracks::iterator it = videoTracks.find(ssrc);
				//If found
				if (it!=videoTracks.end())
					//Get it
					videoTrack = it->second;
				//Convert to video frame
				VideoFrame &videoFrame = (VideoFrame&) frame;

				//If it is intra
				if (waitVideo  && videoFrame.IsIntra())
				{
					//Don't wait more
					waitVideo = 0;
					//Set first timestamp
					first = time;
				}
				
				//Check if it is the first
				if (first==(QWORD)-1)
					//Set this one as first
					first = time;
			
				// Calculate new timestamp
				QWORD timestamp = time-first;

				//Check if we have to write or not
				if (!waitVideo)
				{
					// Check if we have the audio track
					if (!videoTrack)
					{
						//Create object
						videoTrack = new mp4track(mp4);
						//Create track
						videoTrack->CreateVideoTrack(videoFrame.GetCodec(),videoFrame.GetWidth(),videoFrame.GetHeight());
						//Add it to map
						videoTracks[ssrc] = videoTrack;
						//If not the first one
						if (timestamp)
						{
							//Create empty video frame
							VideoFrame empty(videoFrame.GetCodec(),0);
							//Set empty data
							empty.SetTimestamp(0);
							//Set duration until first real frame
							empty.SetDuration(timestamp);
							//Send first empty packet
							videoTrack->WriteVideoFrame(empty);
						}
					}
					
					//Update timestamp
					//TODO: Don't do it
					videoFrame.SetTimestamp(timestamp);

					// Save audio rtp packet
					videoTrack->WriteVideoFrame(videoFrame);
				}
				break;
			}
			case MediaFrame::Unknown:
				//Nothing
				break;
		}
	}

	//Unlock the  access to the file
	pthread_mutex_unlock(&mutex);
	
}


