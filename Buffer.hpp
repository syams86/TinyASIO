#pragma once
#include <vector>
#include <Windows.h>
#include <algorithm>
#include <array>

#include "Option.hpp"
#include "SDK.hpp"
#include "Driver.hpp"
#include "Channel.hpp"

namespace asio
{
	

	/**
	* バッファ用のクラス
	*/
	class BufferBase
	{
	protected:
		void *buffers[2];	//!< バッファ
		long channelNumber;	//!< チャンネル番号

		StreamingVector stream;		//!< ストリーミング用の変数
		CRITICAL_SECTION critical;	//!< クリティカルセクション


		template <typename FUNC>
		void Critical(FUNC func)
		{
			EnterCriticalSection(&critical);
			func();
			LeaveCriticalSection(&critical);
		}


	public:
		BufferBase(const ASIOBufferInfo& info)
			: channelNumber(info.channelNum)
		{
			buffers[0] = info.buffers[0];
			buffers[1] = info.buffers[1];

			stream = StreamingVector(new std::vector<int>());
			InitializeCriticalSection(&critical);
		}

		
		virtual ~BufferBase()
		{
			DeleteCriticalSection(&critical);
		}


		inline const long ChannelNumber() const { return channelNumber; }	//!< チャンネル番号
		inline void* GetBuffer(const long index) { return buffers[index]; }	//!< indexからバッファを取得する


		/**
		* バッファの中身を取り出す
		* @return バッファの中身
		*/
		StreamingVector Fetch()
		{
			StreamingVector retval = stream;
			Critical([&](){ stream = StreamingVector(new std::vector<int>()); });
			return retval;
		}


		/**
		* 元からあるバッファに転送する
		* @param[in,out] buffer 転送したいバッファ
		* @param[in] bufferLength バッファの長さ
		*/
		void Fetch(void* buffer, const long bufferLength)
		{
			Critical([&](){
				long length = bufferLength;
				if (length > stream->size())
					length = stream->size();
				memcpy(buffer, &stream->at(0), length * sizeof(int));
				stream->erase(stream->begin(), stream->begin() + length);
			});
		}


		/**
		* バッファに値を蓄積する
		* @param[in] store 蓄積したい値
		*/
		void Store(const std::vector<int>& store)
		{
			Critical([&](){stream->insert(stream->end(), store.begin(), store.end()); });
		}


		/**
		* void*からバッファに蓄積する
		* @param[in] buffer 移したいバッファ
		* @param[in] bufferLength バッファの長さ
		*/
		void Store(void* buffer, const long bufferLength)
		{
			int* ptr = reinterpret_cast<int*>(buffer);
			Critical([&](){ stream->insert(stream->end(), ptr, ptr + bufferLength); });
		}

		/**
		* チャンネル番号で比較する
		*/
		inline const bool IsChannelNumber(const long channelNumber) const
		{
			return this->channelNumber == channelNumber;
		}

		/**
		* チャンネル番号で比較する
		*/
		inline const bool IsChannelNumber(const Channel& channel) const
		{
			return channelNumber == channel.channelNumber;
		}

		/**
		* バッファの領域がnullじゃなかったらtrue
		*/
		inline const bool IsEnabledBuffer() const
		{
			return buffers[0] != nullptr && buffers[1] != nullptr;
		}
	};


	/**
	* 入力バッファ, ギターやマイクなどの入力を扱う
	*/
	class InputBuffer : public BufferBase
	{
	public:
		InputBuffer(const ASIOBufferInfo& info)
			: BufferBase(info) {}
	};


	/**
	* 出力バッファ，ヘッドフォンやスピーカーなどへ出力する
	*/
	class OutputBuffer : public BufferBase
	{
	public:
		OutputBuffer(const ASIOBufferInfo& info)
			: BufferBase(info) {}
	};


	/**
	* バッファの管理クラス
	*/
	class BufferManager
	{
		std::vector<ASIOBufferInfo> bufferInfo;

		std::vector<BufferBase> buffers;
		std::vector<InputBuffer> inputBuffers;
		std::vector<OutputBuffer> outputBuffers;

		static std::vector<BufferBase>* buffersPtr;			//!< コールバック関数から使えるようにするためのポインタ
		static std::vector<InputBuffer>* inputBuffersPtr;
		static std::vector<OutputBuffer>* outputBuffersPtr;

	private:
		template <typename VECTOR_ARRAY>
		void InitBufferInfo(const VECTOR_ARRAY& channels)
		{
			bufferInfo = std::vector<ASIOBufferInfo>(channels.size());

			for (int i = 0; i < channels.size(); ++i)
			{
				bufferInfo[i].channelNum = channels[i].channelNumber;
				bufferInfo[i].isInput = channels[i].isInput;
			}
		}

		void InitBuffers(const long bufferLength, ASIOCallbacks* callbacks)
		{
			auto* iasio = Driver::Get().Interface();
			ErrorCheck(iasio->createBuffers(&bufferInfo[0], bufferInfo.size(), bufferLength, callbacks));

			for (long i = 0; i < bufferInfo.size(); ++i)
			{
				if (bufferInfo[i].isInput)
					inputBuffers.emplace_back(bufferInfo[i]);
				else
					outputBuffers.emplace_back(bufferInfo[i]);
			}

			inputBuffersPtr = &inputBuffers;
			outputBuffersPtr = &outputBuffers;
		}

	public:
		BufferManager(const std::vector<Channel> channels, const long bufferLength, ASIOCallbacks* callbacks)
		{
			InitBufferInfo(channels);
			InitBuffers(bufferLength, callbacks);
		}

		template <size_t NUM>
		BufferManager(const std::array<Channel, NUM> channels, const long bufferLength, ASIOCallbacks* callbacks)
		{
			InitBufferInfo(channels);
			InitBuffers(bufferLength, callbacks);
		}

		/**
		* バッファリングされている入力チャンネルを探す
		* もっとも最初に見つかったものが返される
		* @note バッファへのvoid*がnullptrじゃないものを取得する
		*/
		InputBuffer& SearchBufferableInput()
		{
			return *std::find_if(inputBuffers.begin(), inputBuffers.end(),
				[](const BufferBase& buffer) -> bool {
				return buffer.IsEnabledBuffer();
			});
		}

		/**
		* バッファリングされている出力チャンネルを探す
		* もっとも最初に見つかったものが返される
		* @note バッファへのvoid*がnullptrじゃないものを取得する
		*/
		OutputBuffer& SearchBufferableOutput()
		{
			return *std::find_if(outputBuffers.begin(), outputBuffers.end(),
				[](const BufferBase& buffer) -> bool {
				return buffer.IsEnabledBuffer();
			});
		}

		static std::vector<InputBuffer>* InputBuffers() { return inputBuffersPtr; }		//!< 公開されている入力バッファを得る
		static std::vector<OutputBuffer>* OutputBuffers() { return outputBuffersPtr; }	//!< 公開されている出力バッファを得る
		static InputBuffer& InputBuffers(const size_t i) { return inputBuffersPtr->at(i); }		//!< 添字から入力バッファを得る
		static OutputBuffer& OutputBuffers(const size_t i) { return outputBuffersPtr->at(i); }	//!< 添字から出力バッファを得る
	};

	std::vector<BufferBase>* BufferManager::buffersPtr = nullptr;
	std::vector<InputBuffer>* BufferManager::inputBuffersPtr = nullptr;
	std::vector<OutputBuffer>* BufferManager::outputBuffersPtr = nullptr;
}