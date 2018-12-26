# VP8 face ROI
This projects demoes the contente aware encoding for real time conferencing, using the VP8 encoder regions of interest to provide more quality to detected faces in video without increasing bandwidth.

## Compile

You would need to compile and install the [vp8 enabled mp4 lib](https://github.com/medooze/mp4v2) first. Then:
```
mkdir build
cd build/
cmake ..
```

## Usage

```
VP8 Face ROI demo
Usage: vp8faceroi [-h] [--help] [-i|--input input-mp4] [-o|--output output.mp4] [--no-roi] [--rate kbps] [--cascade cascade.xml]

Options:
 -h,--help        Print help
 -f               Run as daemon in safe mode
 -i,--input       Input file (default: face.mp4)
 -o,--oputut      Output file (default: outp.mp4)
 --no-roi         Disable ROI settings
 --rate           Encoder bitrate (default: 512kbps)
 --cascade        Haar Cascade file (default: haarcascade_frontalface_default.xml)
 --from           First frame to start coding (default:0)
 --to             Last frame to encode (default:600)
root@ubuntu:/usr/local/src/face_detect_n_track/build#
```

### Face detection code

We use the [ast and robust face detection and tracking code](https://github.com/hrastnik/face_detect_n_track)

### License
MIT
