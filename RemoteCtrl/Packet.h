#pragma once
#include "pch.h"
#include "framework.h"

#pragma pack(push)
#pragma pack(1)

class CPacket {
public:
	CPacket() :sHead{ 0 }, nLength{ 0 }, sCmd{ 0 }, sSum{ 0 } {}
	CPacket(WORD nCmd, const BYTE* pData, size_t nSize) {
		sHead = 0xFEFF;
		nLength = nSize + 4;
		sCmd = nCmd;
		if (nSize > 0) {
			strData.resize(nSize);
			memcpy(&(*strData.begin()), pData, nSize);
		}
		else {
			strData.clear();
		}

		sSum = 0;
		for (size_t j{ 0 }; j < strData.size(); ++j) {
			sSum += BYTE(strData[j]) & 0xFF;
		}
	}
	CPacket(const BYTE* pData, size_t& nSize) {
		size_t i{ 0 };

		// sHead
		for (; i < nSize; ++i) {
			if (*(WORD*)(pData + i) == 0xFEFF) {
				sHead = *(WORD*)(pData + i);
				i += 2;			// 跳过匹配到的 FEFF
				break;
			}
		}
		if (i + 4 + 2 + 2 > nSize) {
			nSize = 0;
			return;
		}

		// nLength
		nLength = *(DWORD*)(pData + i);
		i += 4;
		if (nLength + i > nSize) {	// 包数据可能不全,或者包数据未能全部接收到
			nSize = 0;
			return;
		}

		// sCmd
		sCmd = *(WORD*)(pData + i);
		i += 2;

		// strData
		strData.resize(nLength - 2 - 2);	// 减去sCmd,和sSum的长度
		if (nLength > 4) {
			memcpy(&(*strData.begin()), pData + i, strData.size());
			i += nLength - 4;
		}

		// sSum
		sSum = *(WORD*)(pData + i);
		i += 2;

		// 校验
		WORD sum{ 0 };
		for (size_t j{ 0 }; j < strData.size(); ++j) {
			sum += BYTE(strData[j]) & 0xFF;
		}
		if (sum == sSum) {
			nSize = i;	// head2 length4 data...
			return;
		}
		nSize = 0;
		return;
	}
	CPacket(const CPacket& pack) :sHead{ pack.sHead }, nLength{ pack.nLength }, sCmd{ pack.sCmd }, sSum{ pack.sSum } {
		strData = pack.strData;
	}
	CPacket& operator = (const CPacket& pack) {
		if (&pack == this) {
			return *this;
		}

		sHead = pack.sHead;
		nLength = pack.nLength;
		sCmd = pack.sCmd;
		strData = pack.strData;
		sSum = pack.sSum;
		return *this;
	}

	size_t size() const {		// 包数据的大小
		return nLength + 6;
	}

	const char* operator()() {
		strOut.resize(nLength + 6);
		BYTE* pData = (BYTE*)strOut.data();
		*(WORD*)pData = sHead;
		pData += 2;
		*(DWORD*)pData = nLength;
		pData += 4;
		*(WORD*)pData = sCmd;
		pData += 2;
		memcpy(pData, strData.c_str(), strData.size());
		pData += strData.size();
		*(WORD*)pData = sSum;
		return strOut.c_str();
	}

	~CPacket() {}
public:
	WORD sHead;		// 固定位 FE FF
	DWORD nLength;	// 包长度(从控制命令开始,到和校验结束)
	WORD sCmd;	// 控制命令
	std::string strData;	// 包数据
	WORD sSum;	// 和校验

	std::string strOut;	// 整个包的数据
};
#pragma pop

typedef struct file_info {
	file_info() {
		IsInvaild = FALSE;
		IsDirectory = FALSE;
		HasNext = TRUE;
		memset(szFileName, 0, sizeof(szFileName));
	}

	BOOL IsInvaild;		// 是否有效
	BOOL IsDirectory;	// 是否为目录 0 否 1 是
	BOOL HasNext;		// 是否还有后续 0 没有 1 有
	char szFileName[256]; // 文件名
}FILEINFO, * PFILEINFO;

typedef struct MouseEvent {
	MouseEvent() {
		nAction = 0;
		nButton = -1;
		ptXY.x = 0;
		ptXY.y = 0;
	}

	WORD nAction;	// 点击,移动，双击
	WORD nButton;	// 左键,右键，中键
	POINT ptXY;		// 坐标
}MOUSEEV, * PMOUSEEV;