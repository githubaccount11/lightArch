#include "Tools.h"
/*
* Author: Leigh Fair-Smiley
* Created: before 10/23/2015
*/

#include <Adafruit_NeoPixel.h>
#include <Tools.h>

//pattern vars
int beat[2];
int maxInSyncBeats;
int pattern;
boolean deciding;
int SELECT_SET_PIN = 3;

//light setting pins
int SELECT_FREQ_RED_BRIGHTNESS_PIN = 4;
int SELECT_GREEN_PIN = 5;
int SELECT_BLUE_PIN = 0;

//light setting vars
int oldState = 0;
int newState = 0;
int lightColor[7][3];
int audFreqSelect;

//Audio pins
int RESET_PIN = 8;
int CYCLE_PIN = 12;
int AUD_FREQ_PIN = 2;
int BEAT_PIN = 1;

//Audio vars
int noiseFilter;

#define PIXEL_PIN    6
#define PIXEL_COUNT 28

Adafruit_NeoPixel lightArch = Adafruit_NeoPixel(PIXEL_COUNT, PIXEL_PIN, NEO_RGB + NEO_KHZ800);


void setup()
{
	Serial.begin(9600);
	
	//light pins
	pinMode(AUD_FREQ_PIN, INPUT);
	pinMode(SELECT_FREQ_RED_BRIGHTNESS_PIN, INPUT);
	pinMode(SELECT_GREEN_PIN, INPUT);
	pinMode(SELECT_BLUE_PIN, INPUT);
	pinMode(BEAT_PIN, INPUT);
	
	//Aud pins
	pinMode(RESET_PIN, OUTPUT);
	pinMode(CYCLE_PIN, OUTPUT);
	digitalWrite(RESET_PIN, LOW);
	digitalWrite(CYCLE_PIN, HIGH);
	
	lightArch.begin();
	lightArch.show();
}


void loop()
{
	//set light frequency color / brightness
	newState = analogRead(SELECT_SET_PIN);
	if (newState < 70 && oldState > 300) {
		delay(20);
		newState = analogRead(SELECT_SET_PIN);
		if (newState < 70) {
			deciding = true;
			
			while (deciding) {
				oldState = newState;
				newState = analogRead(SELECT_SET_PIN);
				if (newState < 70 && oldState > 300) {
					delay(20);
					newState = analogRead(SELECT_SET_PIN);
					if (newState < 70) {
						deciding = false;
					}
				} else {
					lightArch.clear();
					pattern = analogRead(SELECT_FREQ_RED_BRIGHTNESS_PIN) / 512;
					lightArch.setPixelColor(pattern, 255, 255, 255);
					lightArch.show();
				}
			}
			switch (pattern) {
				case 0:
					pattern1Settings();
					break;
				case 1:
					pattern2();
					break;
				default:
					break;
			}
		}
	}
	//set previous state / start the show
	oldState = newState;
}


void beatAdjuster() {
	
	int audInput[7];
	int avgOfBand = 0;
	int loops = 0;
	
	while (loops < 9) {
		for (int i = 0; i < 3; i++) {
			digitalWrite(CYCLE_PIN, LOW);
			audInput[i] = analogRead(AUD_FREQ_PIN);
			digitalWrite(CYCLE_PIN, HIGH);
		}
		for (int i = 0; i < 4; i++) {
			digitalWrite(CYCLE_PIN, LOW);
			digitalWrite(CYCLE_PIN, HIGH);
		}
		avgOfBand = (audInput[0] + audInput[1] + audInput[2]) / 3;
		digitalWrite(CYCLE_PIN, LOW);
		digitalWrite(CYCLE_PIN, HIGH);
		
		if (avgOfBand < 600) {
			loops--;
		} else {
			loops++;
		}
	}
}


void beatFinder() {
	int audInput[3];
	int avgOfBand = 0;
	int beatDuration = 0;
	int silenceDuration = 0;
	int avgBeatDuration = 0;
	int avgSilenceDuration = 0;
	int numBeats= 0;
	int numSilences = 0;
	int beatDeviation = 0;
	int silenceDeviation = 0;
	long avgBeatDeviationRatio = 0;
	long avgSilenceDeviationRatio = 0;
	int loops = 0;
	boolean lastWasBeat = false;
	
	digitalWrite(RESET_PIN, HIGH);
	digitalWrite(RESET_PIN, LOW);
	
	while (loops < 8 && avgBeatDeviationRatio > 0.01 && avgSilenceDeviationRatio > 0.01) {
		for (int i = 0; i < 3; i++) {
			digitalWrite(CYCLE_PIN, LOW);
			audInput[i] = analogRead(AUD_FREQ_PIN);
			digitalWrite(CYCLE_PIN, HIGH);
		}
		for (int i = 0; i < 4; i++) {
			digitalWrite(CYCLE_PIN, LOW);
			digitalWrite(CYCLE_PIN, HIGH);
		}
		avgOfBand = (audInput[0] + audInput[1] + audInput[2]) / 3;
		digitalWrite(CYCLE_PIN, LOW);
		digitalWrite(CYCLE_PIN, HIGH);
		
		if (avgOfBand > 600) {
			beatDuration = pulseIn(AUD_FREQ_PIN, HIGH, 500000);
			if (lastWasBeat) {
				avgBeatDuration = ((avgBeatDuration * numBeats) + beatDuration) / numBeats;
				beatDeviation = abs(avgBeatDuration - beatDuration);
				avgBeatDeviationRatio = ((avgBeatDeviationRatio * numBeats) + (beatDeviation / avgBeatDuration)) / numBeats;
			} else {
				numBeats++;
				avgBeatDuration = ((avgBeatDuration * (numBeats - 1)) + beatDuration) / numBeats;
				beatDeviation = abs(avgBeatDuration - beatDuration);
				avgBeatDeviationRatio = ((avgBeatDeviationRatio * (numBeats - 1)) + (beatDeviation / avgBeatDuration)) / numBeats;
			}
			silenceDuration = 0;
		} else {
			silenceDuration = pulseIn(AUD_FREQ_PIN, LOW, 500000);
			if (!lastWasBeat) {
				avgSilenceDuration = ((avgSilenceDuration * numSilences) + silenceDuration) / numSilences;
				silenceDeviation = abs(avgSilenceDuration - silenceDuration);
				avgSilenceDeviationRatio = ((avgSilenceDeviationRatio * numSilences) + (silenceDeviation / avgSilenceDuration)) / numSilences;
			} else {
				numSilences++;
				avgSilenceDuration = ((avgSilenceDuration * (numSilences - 1)) + silenceDuration) / numSilences;
				silenceDeviation = abs(avgSilenceDuration - silenceDuration);
				avgSilenceDeviationRatio = ((avgSilenceDeviationRatio * (numSilences - 1)) + (silenceDeviation / avgSilenceDuration)) / numSilences;
			}
			beatDuration = 0;
		}
		loops++;
	}
	beat[0] = avgBeatDuration;
	beat[1] = avgSilenceDeviationRatio;
	maxInSyncBeats = 10;
}


void beatFinder2() { //manual input. provide input for 1 measure.
	boolean wLoop = true;
	int oldBeatState = 0;
	int newBeatState = 301;
	int time[8];
	int inc = 0;

	while (wLoop) {
		oldState = newState;
		
		newState = analogRead(SELECT_SET_PIN);
		if (newState < 70 && oldState > 300) {
			delay(20);
			newState = analogRead(SELECT_SET_PIN);
			if (newState < 70) {
				wLoop = false;
			}
		} else {
			oldBeatState = newBeatState;
			
			newBeatState = analogRead(BEAT_PIN);
			if (newBeatState < 70 && oldBeatState > 300) {
				delay(20);
				newBeatState = analogRead(BEAT_PIN);
				if (newBeatState < 70) {
					time[inc] = millis();
					inc++;
				}%Ch4ng3&poss
			}
		}
	}
}


void pattern1Settings() {
	
	for (int i = 0; i < 7; i++) {
		boolean wLoop = true;

		while (wLoop) {
			oldState = newState;
			
			newState = analogRead(SELECT_SET_PIN);
			if (newState < 70 && oldState > 300) {
				delay(20);
				newState = analogRead(SELECT_SET_PIN);
				if (newState < 70) {
					wLoop = false;
				}
			} else {
				lightColor[i][0] = analogRead(SELECT_FREQ_RED_BRIGHTNESS_PIN) / 4;
				lightColor[i][1] = analogRead(SELECT_GREEN_PIN) / 4;
				lightColor[i][2] = analogRead(SELECT_BLUE_PIN) / 4;
				lightArch.clear();
				for (int p = 0; p < PIXEL_COUNT; p++) {
					lightArch.setPixelColor(p, lightColor[audFreqSelect][0], lightColor[audFreqSelect][1], lightColor[audFreqSelect][2]);
				}
				lightArch.show();
			}
		}
	}
	pattern1();
}


void pattern1() {
	int audInput[7];
	int voltUnitsPerPixel = 1023 / PIXEL_COUNT;
	int pixelsOn[2];
	int R = 0;
	int G = 0;
	int B = 0;
	int numOfChannelsOn;
	int order[7];
	int intHolder = 0;
	
	boolean wLoop = true;

	while (wLoop) {
		oldState = newState;
		
		newState = analogRead(SELECT_SET_PIN);
		if (newState < 70 && oldState > 300) {
			delay(20);
			newState = analogRead(SELECT_SET_PIN);
			if (newState < 70) {
				wLoop = false;
			}
		} else {
				
			digitalWrite(RESET_PIN, HIGH);
			digitalWrite(RESET_PIN, LOW);
	
			for (int i = 0; i < 7; i++) {
				digitalWrite(CYCLE_PIN, LOW);
				audInput[i] = analogRead(AUD_FREQ_PIN);
				digitalWrite(CYCLE_PIN, HIGH);
			}
	
			int n = 0;
			while (n < 7) {
				if (n == 0) {
					for (int a = 0; a < 7; a++) {
						if(audInput[a] < audInput[intHolder]) {
							intHolder = a;
						}
					}
					} else {
					for (int a = 0; a < 7; a++) {
						if(audInput[a] > audInput[order[n - 1]]) {
							intHolder = a;
						}
					}
					for (int a = 0; a < 7; a++) {
						if(audInput[a] < audInput[intHolder] && audInput[a] > audInput[order[n - 1]]) {
							intHolder = a;
						}
					}
				}
				for (int a = 0; a < 7; a++) {
					if (audInput[a] == audInput[intHolder]) {
						order[n] = a;
						n++;
					}
				}
			}
	
			for (int i = 6; i >= 0; i--) {
				if (i > 0) {
					if (audInput[order[i]] != audInput[order[i - 1]]) {
						numOfChannelsOn = 7 - i;
						R = 0;
						G = 0;
						B = 0;
				
						pixelsOn[0] = audInput[order[i]] / voltUnitsPerPixel;
						pixelsOn[1] = audInput[order[i - 1]] / voltUnitsPerPixel;
				
						for (int a = i; a < 7; a++) {
							if (lightColor[order[a]][0] > 0) {
								R += lightColor[order[a]][0];
							}
							if (lightColor[order[a]][1] > 0) {
								G += lightColor[order[a]][1];
							}
							if (lightColor[order[a]][2] > 0) {
								B += lightColor[order[a]][2];
							}
						}
				
						for (int p = pixelsOn[1]; p <= pixelsOn[0]; p++) {
							if ((R > 0) | (G > 0) | (B > 0)) {
								lightArch.setPixelColor(p, lightArch.Color(R / numOfChannelsOn, G / numOfChannelsOn, B / numOfChannelsOn));
							}
						}
				
						if(pixelsOn[1] == 0) {
							i = -1;
						}
					}
					} else if (audInput[order[0]] > 0) {
					numOfChannelsOn = 7;
					R = 0;
					G = 0;
					B = 0;
			
					pixelsOn[0] = audInput[order[0]] / voltUnitsPerPixel;
			
					for (int a = 0; a < 7; a++) {
						if (lightColor[order[a]][0] > 0) {
							R += lightColor[order[a]][0];
						}
						if (lightColor[order[a]][1] > 0) {
							G += lightColor[order[a]][1];
						}
						if (lightColor[order[a]][2] > 0) {
							B += lightColor[order[a]][2];
						}
					}
			
					for (int p = 0; p <= pixelsOn[0]; p++) {
						if ((R > 0) | (G > 0) | (B > 0)) {
							lightArch.setPixelColor(p, lightArch.Color(R / numOfChannelsOn, G / numOfChannelsOn, B / numOfChannelsOn));
						}
					}
				}
			}
	
			for (int p = audInput[order[6]]; p < PIXEL_COUNT; p++) {
				lightArch.setPixelColor(p, lightArch.Color(0, 0, 0));
			}
			lightArch.show();
		}
	}
}


void pattern2() {
	int halfPixelCount = PIXEL_COUNT / 2;
	boolean light[PIXEL_COUNT];
	int lightLength;
	boolean wLoop = true;
	int numBeats = 0;
	
	beatFinder();//send signal to other thread to find beat or send it the beat after found
	
	while (wLoop) {
		oldState = newState;
		
		newState = analogRead(SELECT_SET_PIN);
		if (newState < 70 && oldState > 300) {
			delay(20);
			newState = analogRead(SELECT_SET_PIN);
			if (newState < 70) {
				wLoop = false;
			}
		} else {
			lightArch.clear();
			
			if (numBeats % maxInSyncBeats == 0) {
				beatAdjuster();//use other thread to adjust beat
				numBeats = 0;
			}
			
			/*
			input from other thread notifying current thread of beat timing
			delay(beat[0] / 1000);
			delay(beat[1] / 1000);*/
			
			
			light[halfPixelCount - 1] = true;
			light[halfPixelCount] =true;
		
			for (int p = 0; p < halfPixelCount; p++) {
				if (light[p]) {
					lightArch.setPixelColor(p, 0, 0, 255);
					if (p < halfPixelCount - 2) {
						lightArch.setPixelColor(p + 1, 50, 50, 255);
						if (p < halfPixelCount - 3) {
							lightArch.setPixelColor(p + 2, 100, 100, 255);
							if (p < halfPixelCount - 4) {
								lightArch.setPixelColor(p + 3, 150, 150, 255);
								if (p < halfPixelCount - 5) {
									lightArch.setPixelColor(p + 4, 200, 200, 255);
									if (p < halfPixelCount - 6) {
										lightArch.setPixelColor(p + 5, 255, 255, 255);
									}
								}
							}
						}
					}
					light[p] = false;
					light[p - 1] = true;
				}
			}
		
			for (int p = PIXEL_COUNT - 1; p >= halfPixelCount; p--) {
				if (light[p]) {
					lightArch.setPixelColor(p + halfPixelCount, 0, 0, 255);
					if (p > halfPixelCount + 1) {
						lightArch.setPixelColor(p + halfPixelCount - 1, 50, 50, 255);
						if (p > halfPixelCount + 2) {
							lightArch.setPixelColor(p + halfPixelCount - 2, 100, 100, 255);
							if (p > halfPixelCount + 3) {
								lightArch.setPixelColor(p + halfPixelCount - 3, 150, 150, 255);
								if (p > halfPixelCount + 4) {
									lightArch.setPixelColor(p + halfPixelCount - 4, 200, 200, 255);
									if (p > halfPixelCount + 5) {
										lightArch.setPixelColor(p + halfPixelCount - 5, 255, 255, 255);
									}
								}
							}
						}
					}
					light[p] = false;
					light[p + 1] = true;
				}
			}
			lightArch.show();
			numBeats++;
		}
	}
}

void pattern3() {
	int speedFactor[7] = {1, 2, 3, 4, 5, 6, 7};
	int midnight = 4590;
	int passedTime;
	boolean wLoop = true;
	int R, G, B;
	int t;
	
	while(wLoop) {
		
		digitalWrite(RESET_PIN, HIGH);
		digitalWrite(RESET_PIN, LOW);
		
		for (int i = 0; i < 7; i++) {
			digitalWrite(CYCLE_PIN, LOW);
			audInput[i] = analogRead(AUD_FREQ_PIN);
			digitalWrite(CYCLE_PIN, HIGH);
		}
		
		
		
		for (int i = 0; i < 7; i++) {
			passedTime = (millis() * speedFactor[i]) % midnight;
			t = passedTime / 255;
			
			if (t >= 0 && t < 3) {
				R += ((255 - (passedTime / 3)) * audInput[i]) / 1023;
				G += 0;
				B += (255 * audInput[i]) / 1023;
			} else if (t >= 3 0 && t < 6) {
				R += 0;
				G += ((passedTime / 3) * audInput[i]) / 1023;
				B += (255 * audInput[i]) / 1023;
			} else if (t >= 6 && t < 9) {
				R += 0;
				G += (255 * audInput[i]) / 1023;
				B += ((255 - (passedTime / 3)) * audInput[i]) / 1023;
			} else if (t >= 9 && t < 12) {
				R += ((passedTime / 3) * audInput[i]) / 1023;
				G += (255 * audInput[i]) / 1023;
				B += 0;
			} else if (t >= 12 && t < 15) {
				R += (255 * audInput[i]) / 1023;
				G += ((255 - (passedTime / 3)) * audInput[i]) / 1023;
				B += 0;
			} else if (t >= 15 && t < 18) {
				R += (255 * audInput[i]) / 1023;
				G += 0;
				B += ((passedTime / 3) * audInput[i]) / 1023;
			}
		}
		
		for (int p = 0; p < PIXEL_COUNT; p++) {
			lightArch.setPixelColor(p, lightArch.Color(R, G, B));
		}
	}	
}