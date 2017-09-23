// SDS011 dust sensor PM2.5 and PM10
// ---------------------------------
//
// By R. Zschiegner (rz@madavi.de)
// April 2016
//
// Documentation:
//		- The iNovaFitness SDS011 datasheet
//

#if ARDUINO >= 100
	#include "Arduino.h"
#else
	#include "WProgram.h"
#endif

class SDS011 {
	public:
		SDS011(Stream& serial);
		int read(float *p25, float *p10);
		void sleep();
		void wakeup();
	private:
		Stream& sds_data;
};
