 #include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

#include "vp8encoder.h"
#include "mp4recorder.h"
#include "VideoFaceDetector.h"



void BGR2YUV420p(uint8_t *yuv, uint8_t *bgr, const int &width, const int &height)
{
	const size_t image_size = width * height;
	size_t upos = image_size;
	size_t vpos = upos + upos / 4;
	for (size_t i = 0; i < image_size; ++i)
	{
		uint8_t b = bgr[3 * i ];
		uint8_t g = bgr[3 * i + 1];
		uint8_t r = bgr[3 * i + 2];
		yuv[i] = ((66 * r + 129 * g + 25 * b) >> 8) + 16;
		if (!((i / width) % 2) && !(i % 2))
		{
			yuv[upos++] = ((-38 * r + -74 * g + 112 * b) >> 8) + 128;
			yuv[vpos++] = ((112 * r + -94 * g + -18 * b) >> 8) + 128;
		}
	}
}

int main(int argc, char** argv)
{
	//Defaults
	bool roi = true;
	const char* input = "face.mp4";
	const char* output = "out.mp4";
	const char* cascade = "haarcascade_frontalface_default.xml";
	int rate = 512;
	int from = 0;
	int to = 600;
	
	
	//Get all
	for(int i=1;i<argc;i++)
	{
		//Check options
		if (strcmp(argv[i],"-h")==0 || strcmp(argv[i],"--help")==0)
		{
			//Show usage
			printf("VP8 Face ROI demo\r\n");
			printf("Usage: vp8faceroi [-h] [--help] [-i|--input input-mp4] [-o|--output output.mp4] [--no-roi] [--rate kbps] [--cascade cascade.xml]\r\n\r\n"
				"Options:\r\n"
				" -h,--help        Print help\r\n"
				" -f               Run as daemon in safe mode\r\n"
				" -i,--input       Input file (default: face.mp4)\r\n"
				" -o,--oputut      Output file (default: outp.mp4)\r\n"
				" --no-roi         Disable ROI settings\r\n"
				" --rate           Encoder bitrate (default: 512kbps)\r\n"
				" --cascade        Haar Cascade file (default: haarcascade_frontalface_default.xml)\r\n"
				" --from           First frame to start coding (default:0)\r\n"
				" --to             Last frame to encode (default:600)\r\n"
			);
			//Exit
			return 0;
		} else if (strcmp(argv[i],"--no-roi")==0)
			//Do not set roi
			roi = false;
		else if (strcmp(argv[i],"--rate")==0 && (i+1<argc))
			//Get encoder rate
			rate = atoi(argv[++i]);
		else if (strcmp(argv[i],"--from")==0 && (i+1<argc))
			//From frame
			from = atoi(argv[++i]);
		else if (strcmp(argv[i],"--to")==0 && (i+1<argc))
			//To frame
			to = atoi(argv[++i]);
		else if (strcmp(argv[i],"-i")==0 && (i+1<argc))
			//Input
			input = argv[++i];
		else if (strcmp(argv[i],"--input")==0 && (i+1<argc))
			//Input
			input = argv[++i];
		else if (strcmp(argv[i],"-o")==0 && (i+1<argc))
			//Ouptut
			output = argv[++i];
		else if (strcmp(argv[i],"--output")==0 && (i+1<argc))
			//Ouptut
			output = argv[++i];
		else if (strcmp(argv[i],"--cascade")==0 && (i+1<argc))
			//cascade file
			cascade = argv[++i];
	}
	
	// Try opening camera
	cv::Mat frame;
	cv::VideoCapture camera(input);
	VideoFaceDetector detector(cv::String(cascade), camera);
	Properties prop;
	VP8Encoder encoder(prop);
	MP4Recorder recorder;
	
	//Check
	if (!camera.isOpened()) 
	{
		//Error
		fprintf(stderr, "Error opening mp4 input file [input:%s]\n",input);
		exit(1);
	}
	
	if (!detector.isOpened())
	{
		//Error
		fprintf(stderr, "Error opening face detector [cascade:%s]\n",cascade);
		exit(1);
	}
	
	//Open mp4 output
	if (!recorder.Create(output))
	{
		//Error
		fprintf(stderr, "Error opening mp4 ouput file [input:%s]\n",input);
		exit(1);
	}
	
	//Set encoders rate
	encoder.SetFrameRate(30,rate,0);
	
	//Start recording
	recorder.Record();
	
	int num = 0;

	uint8_t* yuv = nullptr;
	
	//Process input file
	while (true)
	{
		auto start = cv::getCPUTickCount();
		//Get next frame and detect face
		if (!(detector >> frame))
			break;
		
		auto end = cv::getCPUTickCount();

		double time_per_frame = (end - start) / cv::getTickFrequency();
		
		double fps = 1/time_per_frame;
		
		//Get size
		size_t width  = frame.size().width;
		size_t height = frame.size().height;
		
		printf("Frame %d time per frame: %3.3f\tFPS: %3.3f face:%s: size:%zux%zu", num, time_per_frame, fps, detector.isFaceFound() ? "yes" : "no", width, height);
		
		if (num<=from)
		{
			printf("\r\n");
			num++;
			continue;
		}

		if (!yuv)
		{
			//Create yuv frame
			yuv = (uint8_t*) malloc(width*height*3/2);
			//Set encoder size
			encoder.SetSize(width,height);
		}
		
		//Check if we have found a face
		if (detector.isFaceFound())
		{
			//Draw it on frame
			cv::rectangle(frame, detector.face(), cv::Scalar(255, 0, 0));
			cv::circle(frame, detector.facePosition(), 30, cv::Scalar(0, 255, 0));
			
			if (roi)
				//Set ROI to the detected face
				encoder.SetRoiMap(detector.face().x, detector.face().y, detector.face().width, detector.face().height);
			
			printf(" face at [%d,%d] size [%dx%d]\r\n",detector.face().x, detector.face().y, detector.face().width, detector.face().height);
		} else {
			
			//encoder.SetRoiMap(0,0,width,height/2);
			if (roi)
				//Disable ROI
				encoder.UnsetRoiMap();
			
			printf("\r\n");
		}
		
		//encoder.SetRoiMap(0,0,width/2,height/2);
		
		//Convert fame to yuv
		BGR2YUV420p(yuv, frame.data, width, height);
		
		//Encode frame
		MediaFrame* encoded = (MediaFrame*)encoder.EncodeFrame(yuv, width*height*3/2);
		
		//Recrodn on mp4
		recorder.onMediaFrame(0u,*encoded,(uint64_t)(((num++-from)*1000)/30));
		
		if (num==to) 
			break;
	}
	
	//Close recorder
	recorder.Close();
	
	//Free frame
	if (yuv)
	{
		//Dump last yuv frame
//		int fd = open("last.yuv", O_WRONLY | O_CREAT, 0644);
//		write(fd, yuv, width*height*3/2);
//		close(fd);
		free(yuv);
	}

	return 0;
}
