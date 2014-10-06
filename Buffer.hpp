#pragma once
#include <vector>
#include <memory>
#include "Interface.hpp"
#include "Structure.hpp"
#include "Driver.hpp"
#include "Channel.hpp"


namespace asio
{
	/**
	* バッファクラス
	*/
	class Buffer
	{
		IOType ioType;
		long channelNumber;
		long bufferSize;
		void* bufferData[2];
		ASIOCallbacks* callbacks;

	public:
		Buffer(const ASIOBufferInfo& info, const long bufferSize, ASIOCallbacks* callbacks)
			: ioType((IOType)info.isInput), channelNumber(info.channelNum), bufferSize(bufferSize), callbacks(callbacks)
		{
			bufferData[0] = info.buffers[0];
			bufferData[1] = info.buffers[1];
		}
	};

	typedef std::vector<Buffer> BufferArray;

	/**
	* バッファを管理するクラス
	*/
	class BufferManager
	{
		IASIO* iasio;

		BufferArray buffers;
		std::vector<ASIOBufferInfo> bufferInfos;

	private:
		void InitBuffers(const long& bufferSize, ASIOCallbacks* callbacks)
		{
			buffers.clear();
			for (unsigned i = 0; i < bufferInfos.size(); ++i)
			{
				const auto& info = bufferInfos[i];
				buffers.emplace_back(info, bufferSize, callbacks);
			}
		}

	public:
		BufferManager(IASIO* iasio)
			: iasio(iasio)
		{

		}

		~BufferManager()
		{
			if (bufferInfos.size() > 0)
				ErrorCheck(iasio->disposeBuffers());
		}

		/**
		* バッファリングしたいチャンネルを追加
		*/
		void AddChannel(const IOType& ioType, const long& channelNumber)
		{
			ASIOBufferInfo info;
			info.channelNum = channelNumber;
			info.isInput = ioType;
			bufferInfos.push_back(info);
		}

		/**
		* バッファリングしたいチャンネルを追加
		*/
		void AddChannel(const Channel& channel)
		{
			AddChannel(channel.ioType, channel.ioType);
		}

		/**
		* バッファリングしたいチャンネルをやっぱなしにする
		*/
		void ClearChannel()
		{
			bufferInfos.clear();
		}

		/**
		* バッファの生成
		* @params[in] bufferSize バッファの設定
		* @params[in, out] callbacks 自己責任，バッファリング等の通知のために利用される
		* @note bufferSizeは自由に数値を決められないので注意, (bufferSize % granularity == 0)以外の数値は保障できない
		*/
		const BufferArray& CreateBuffer(const long& bufferSize, ASIOCallbacks* callbacks = nullptr)
		{
			ErrorCheck(iasio->createBuffers(&bufferInfos.at(0), bufferInfos.size(), bufferSize, callbacks));
			InitBuffers(bufferSize, callbacks);
			return buffers;
		}

		/**
		* バッファの生成
		* @params[in] bufferPreference バッファの設定
		* @params[in, out] callbacks 自己責任，バッファリング等の通知のために利用される
		* @note bufferSizeは自由に数値を決められないので注意, (bufferSize % granularity == 0)以外の数値は保障できない
		*/
		const BufferArray& CreateBuffer(const BufferPreference& bufferPreference, ASIOCallbacks* callbacks = nullptr)
		{
			CreateBuffer(bufferPreference.preferredSize, callbacks);
			return buffers;
		}

		/**
		* 明示的にバッファを解放
		*/
		void DisposeBuffers() const
		{
			ErrorCheck(iasio->disposeBuffers());
		}
		
	};
}