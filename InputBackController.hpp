#pragma once

#include "ControllerBase.hpp"
#include "Channel.hpp"

namespace asio
{
	/* 入力信号を出力にそのまま返す */
	class InputBackController : public ControllerBase
	{
		static InputBuffer* input;
		static OutputBuffer* output;

	private:
		static void BufferSwitch(long index, long directProcess)
		{
			void* outBuf = output->GetBuffer(index);
			void* inBuf = input->GetBuffer(index);

			memcpy(outBuf, inBuf, bufferLength * sizeof(int));	// 入力のバッファを出力へ移す

			input->Store(inBuf, bufferLength);
		}

	public:
		/**
		* 指定したチャンネルからコントローラを生成する
		*/
		InputBackController(const InputChannel& inputChannel, const OutputChannel& outputChannel)
			: ControllerBase() 
		{
			input = &bufferManager->Search(inputChannel);
			output = &bufferManager->Search(outputChannel);
		}

		/**
		* バッファリング可能なチャンネルからコントローラを生成する
		*/
		InputBackController()
			: ControllerBase()
		{
			input = &bufferManager->SearchBufferableInput();
			output = &bufferManager->SearchBufferableOutput();
		}

		
	};

	InputBuffer* InputBackController::input = nullptr;
	OutputBuffer* InputBackController::output = nullptr;
}