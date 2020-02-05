#include <iostream>
#include <stdio.h>
#include <tchar.h>
#include <fftw3.h>
#include <chrono>
#include <thread>
#include <vector>
#include <string>
//#include <SFML/Window.hpp>
//#include <SFML/Graphics/VertexArray.hpp>
#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>

using namespace std; 

#define MUSICPLAYER false
#define REAL 0
#define IMAG 1
#define SAMPLE_RATE 44100 // for the audio, in Hz
#define SAMPLE_LENGTH 20 // for the plotting, in milliseconds
//length of complex arrays
//#define N 8

const int spaceBetweenBins = 2; 
//const int fft_ArrayLength = SAMPLE_RATE * (SAMPLE_LENGTH / 1000 + 850); 
const int fft_ArrayLength = SAMPLE_LENGTH/1000 + 1000; 
const int nBins = 500; // For accuracy, this should be equal to fft_ArrayLength; 
//Function to compute 1-D FFT
void fft(fftw_complex* in, fftw_complex* out, const int sampleCount)
{
	//create a DFT plan
	fftw_plan plan = fftw_plan_dft_1d(fft_ArrayLength, in, out, FFTW_FORWARD, FFTW_ESTIMATE);
	//execute plan
	fftw_execute(plan); 
	//clean up
	fftw_destroy_plan(plan); 
	fftw_cleanup(); 
}

//function to display complex numbers
void displayComplex(fftw_complex* y, int N)
{
	for (int i = 0; i < N; ++i)
	{
		if (y[i][IMAG] < 0) 
			cout << y[i][REAL] << " - " << abs(y[i][IMAG]) << "i" << endl; 
		else 
			cout << y[i][REAL] << " + " << y[i][IMAG] << "i" << endl; 
		
	}
}

// display only the real part of a complex number
void displayReal(fftw_complex* y)
{
	for (int i = 0; i < 8; ++i)
		cout << y[i][REAL] << endl; 
}

int captureAudio(unsigned int sampleRate, unsigned int intervalTime, sf::SoundBuffer* buffer)
{
	if (sf::SoundRecorder::isAvailable() == false)
	{
		std::cout << "Sorry, audio capture is not supported by your system" << std::endl;
		return EXIT_SUCCESS;
	}

	// Here we'll use an integrated custom recorder, which saves the captured data into a SoundBuffer
	sf::SoundBufferRecorder recorder;

	//recording for intervalTime
	cout << "Audio capture start" << endl; 
	recorder.start(sampleRate);
	std::this_thread::sleep_for(std::chrono::nanoseconds(intervalTime)); // sleep this thread while recording
	recorder.stop();
	cout << "Audio capture finished." << endl; 
	*buffer = recorder.getBuffer(); 
}

int playBuffer(sf::SoundBuffer* buffer)
{
	cout << "Playing " << (*buffer).getDuration().asSeconds() << " second clip. " << endl; 
	sf::Sound sound(*buffer); 
	sound.play(); 
	//wait until sound is over to continue thread
	while (sound.getStatus() == sf::Sound::Playing)
	{
		// Display the playing position
		std::cout << "\rPlaying... " << sound.getPlayingOffset().asSeconds() << " sec        ";
		std::cout << std::flush;

		// Leave some CPU time for other threads
		sf::sleep(sf::milliseconds(100));
	}
	return 1; 
}


class MyMusic : public sf::Music
{
protected:

	bool onGetData(Chunk& data) override
	{
		if (sf::Music::onGetData(data))
		{
			// ... data contains the next chunk of samples to be played by the music ...
			cout << "Test" << endl;
			return true;
		}
		else
			return false;
	}
};


class MyRecorder : public sf::SoundRecorder
{
	/*
	virtual bool onStart() // optional
	{
		// initialize whatever has to be done before the capture starts
		

			// return true to start the capture, or false to cancel it
			return true;
	}*/
public: 
	MyRecorder(sf::Time timeInterval, const unsigned int complexSize): complexSize_m(complexSize)
	{ 
		this->setProcessingInterval(timeInterval);
	}

	~MyRecorder()
	{ 
		delete[] x;
		delete[] y;
		stop(); 
	}

	virtual bool onProcessSamples(const sf::Int16* samples,const std::size_t sampleCount)
	{
		// do something useful with the new chunk of samples
		
		//cout << "Samples Processed..." << endl; 
		cout << "Number of Samples: "<< sampleCount << endl; 

		
		for (int i = 0; i < sampleCount; i++)
		{
			x[i][REAL] = samples[i];
			x[i][IMAG] = 0;
		}

		fft(x, y, sampleCount); // take the fft of x and store in y
		//cout << "Y before send: " << endl; 
		//displayComplex(y, 5); 
			// return true to continue the capture, or false to stop it
			return true;
	}

	
	fftw_complex* getFFTData()
	{
		return y; 
	}

	const unsigned int complexSize_m; 
	fftw_complex* x{ new fftw_complex[complexSize_m] };
	fftw_complex* y{ new fftw_complex[complexSize_m] };
	
	

	/*virtual void onStop() // optional
	{
		// clean up whatever has to be done after the capture is finished
		
	}*/
};




int main() {
	


	////interval time in nanoseconds
	//unsigned int intervalTime = 4000000000;
	//captureAudio(44100, intervalTime, &buffer);
	//playBuffer(&buffer);

	// Testing SoundBufferRecorder Properties
	//sf::SoundBufferRecorder recorder;
	
	
	// Declare and create a new window
	sf::RenderWindow window(sf::VideoMode(1220, 600), "SFML window");
	// Limit the framerate to 60 frames per second (this step is optional)
	//window.setFramerateLimit(60);
	MyRecorder recorder(sf::milliseconds(SAMPLE_LENGTH), fft_ArrayLength); // time between processing intervals in milliseconds
	sf::SoundBuffer buffer;
	//int sampleTime = 1000;
	
	
	// The main loop - ends as soon as the window is closed
	// run the program as long as the window is open
	int samplesPerBin = nBins / fft_ArrayLength + 1; 
	//int spaceBetweenBins = 5; 
	int binWidth = 1100 / nBins;
	int gain = 2;
	fftw_complex* Y{ new fftw_complex[fft_ArrayLength] };


	if (!MUSICPLAYER) {

		cout << recorder.getDefaultDevice() << endl;
		//fftw_complex* Y{ new fftw_complex[fft_ArrayLength] };// the FFT array we will use to plot data

		recorder.start(SAMPLE_RATE); //recorder goes off into a different thread so we can update graphics here
	}
	else
	{
		sf::Music music;
		// Open it from an audio file
		if (!music.openFromFile("blameit.wav"))
		{
			// error...
			cout << "Error loading music." << endl; 
		}
		// Change some parameters
		//music.setPosition(0, 1, 10); // change its 3D position
		//music.setPitch(2);           // increase the pitch
		//music.setVolume(50);         // reduce the volume
		//music.setLoop(true);         // make it loop
		// Play it
		music.play();
		cout << music.getStatus() << endl;
		 
	}

	

	while (window.isOpen())
	{
		// check all the window's events that were triggered since the last iteration of the loop
		sf::Event event;
		while (window.pollEvent(event))
		{
			// "close requested" event: we close the window
			if (event.type == sf::Event::Closed)
				window.close();
		}

		// clear the window with black color
		window.clear(sf::Color::Black);

		// draw everything here...
		// window.draw(...);
		//////////////////////////////////////////////////////////////////////
		
		//query the recorder for new fft data
		if (!MUSICPLAYER)
		{
			Y = recorder.getFFTData();
		}
		else 
		{	//Perform fft on buffered samples from song 
			Y = 0; 
		}


		//instantiate array that will hold rectangles, 4 verticies per bin(rectangle)
		sf::VertexArray rectangles(sf::Quads, 4*nBins);
		
		////////// Try this
		int binAverage = 0;
		
		for (int i = 0; i < nBins; ++i)
		{
			binAverage = 0; 
			for (int j = 0; j < samplesPerBin; j++)
			{
				binAverage += abs(Y[i*samplesPerBin + j][REAL]);
			}
			binAverage = ((double(binAverage)/samplesPerBin - 0)) / (1000) * (1); // linear transform formula 

			// set each bins properties
			rectangles[4 * i + 0].position = sf::Vector2f(binWidth*i + spaceBetweenBins*(i+1), 550.f);
			rectangles[4 * i + 1].position = sf::Vector2f(binWidth *i + spaceBetweenBins* (i + 1), 530 - binAverage*gain);
			rectangles[4 * i + 2].position = sf::Vector2f(binWidth *i + spaceBetweenBins* (i + 1) + binWidth, 530 - binAverage*gain);
			rectangles[4 * i + 3].position = sf::Vector2f(binWidth *i + spaceBetweenBins* (i + 1) + binWidth, 550.f);
				
			rectangles[4 * i + 0].color = sf::Color::Blue;
			rectangles[4 * i + 1].color = sf::Color::Blue;
			rectangles[4 * i + 2].color = sf::Color::Blue;
			rectangles[4 * i + 3].color = sf::Color::Blue;
			
			//cout << "Bin ave: " << binAverage << endl; 
			if (i % 3 == 1)
				//cout << "Mod var bin average: " << binAverage << endl;
			binAverage = 0; 
			
				
		}
		
		// define the position of the triangle's points
		/*
		rectangles[0].position = sf::Vector2f(100.f, 500.f);
		rectangles[1].position = sf::Vector2f(100.f, 450-tempOutput*100);
		rectangles[2].position = sf::Vector2f(200.f, 450-tempOutput*100);
		rectangles[3].position = sf::Vector2f(200.f, 500.f);

		rectangles[0].color = sf::Color::Blue; 
		rectangles[1].color = sf::Color::Blue;
		rectangles[2].color = sf::Color::Blue;
		rectangles[3].color = sf::Color::Blue;
		*/
		//////////////////////////////////////////////////////////////////////
		window.draw(rectangles);
		// end the current frame
		window.display();
		sf::sleep(sf::milliseconds(SAMPLE_LENGTH / 4));
	}
	delete[] Y; 
	/*
	Accessing raw audio example
	const sf::Int16* samples = buffer.getSamples();
	std::size_t count = buffer.getSampleCount();
	doSomething(samples, count);
	*/

	/*
	
	
	*/

	return 0; 
}