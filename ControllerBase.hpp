#pragma once
#include <string>

#include "Driver.hpp"

namespace asio
{
	class ControllerBase
	{
	protected:
		Driver* driver;

		long bufferLength;
		long inputLatency;
		long outputLatency;
		long sampleRate;

	protected:
		ControllerBase()
		{
			driver = &Driver::Get();
			auto *iasio = driver->Interface();

			iasio->getBufferSize(NULL, NULL, &bufferLength, NULL);
			iasio->getLatencies(&inputLatency, &outputLatency);

			double sr;	// double型はなんか不自然なので変換する
			iasio->getSampleRate(&sr);
			sampleRate = (long)sr;
		}

	public:
		void Start() { driver->Interface()->start(); }	//!< 録音開始
		void Stop() { driver->Interface()->stop(); }	//!< 録音終了
		
		inline const long BufferLength() const { return bufferLength; }		//!< バッファの数を返す
		inline const long InputLatency() const { return inputLatency; }		//!< 入力の遅延を返す
		inline const long OutputLatency() const { return outputLatency; }	//!< 出力の遅延を返す
	};
}