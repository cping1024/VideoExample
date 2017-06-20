#include <stdio.h>
#include <string>
#include <util/sn_video_frame_provider.h>
int main(int argc, char* argv[]) {

    if (argc < 2) {
		printf("Args error!\n");
		return -1;
	}

	const char* url = argv[1];
    printf("url [%s].\n", url);

    SNVideoFrameReader reader;
    int ret = reader.initReader(std::string(url));
	if (ret != 0) {
		printf ("init videoframe provider fail!\n");
		return -1;
	}

    ret = reader.start();
	if (ret != 0) {
		printf("start provider fail!\n");
        reader.deInitReader();
		return -1;
	}
	
	/// test blocked mode
	printf("========================================================\n");
	const int frame_num = 100;
	for (int ix = 0; ix < frame_num; ++ix) {
		void* buf = nullptr;
		int size = 0;

        ret = reader.readVideoFrame(&buf, &size);
		if (ret != 0) {
			printf ("get video frame fail , num [%d]!\n", ix);		
		} else {
			printf ("video frame size [%d]!\n", size);
		}		
	} 

	printf("Enter Any Key to exit!\n");
	getchar();
	/*	
	printf("========================================================\n");
	/// test non_blocked mode
        for (int ix = 0; ix < frame_num; ++ix) {
                void* buf = nullptr;
                int size = 0;

                ret = provider.getVideoFrame(&buf, &size, false);
                if (ret != 0) {
                        printf ("get video frame fail , num [%d]!\n", ix);      
                } else {
                        printf ("video frame size [%d]!\n", size);
                }               
        } 
	*/
	/// stop provider
    reader.stop();

	printf ("Exit provider test!\n");	
	return 0;
}
