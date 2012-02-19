#include "StdAfx.h"
#include "xDeviceFileStream.h"

namespace Streams
{
xDeviceFileStream::xDeviceFileStream(string Path, Drive *device)
{
    File *dest = device->FileFromPath(Path);
    Initialize(dest, device);
}

xDeviceFileStream::xDeviceFileStream(File *dest, Drive *device)
{
    Initialize(dest, device);
}

xDeviceFileStream::~xDeviceFileStream(void)
{
}

void xDeviceFileStream::Initialize(File *xf, Drive *device)
{
    IsClosed = false;
    _Endian = Big;
    this->xf = xf;
    this->device = device;
    if (!xf->ClusterChain.size())
    {
        device->ReadClusterChain(xf->ClusterChain, xf->Dirent, *xf->Volume);
    }
    SetPosition(0);
}

void xDeviceFileStream::Close( void )
{
    IsClosed = true;
}

INT64 xDeviceFileStream::Position( void )
{
    return UserPosition;
}

INT64 xDeviceFileStream::Length( void )
{
    return (INT64)xf->Dirent.FileSize;
}

void xDeviceFileStream::SetPosition( INT64 Position )
{
    device->DeviceStream->SetPosition(GetPhysicalPosition(Position));
    qDebug("Device filestream position set: 0x%X\nPhysical disk position:0x%X", Position, GetPhysicalPosition(Position));
    UserPosition = Position;
}

INT64 xDeviceFileStream::GetPhysicalPosition(int FilePosition)
{
    int ClusterIndex = Helpers::UpToNearestX(FilePosition, xf->Volume->ClusterSize) / xf->Volume->ClusterSize;
    int Index = 0;
    for (; Index < ClusterIndex - 1; Index++)
    {
        FilePosition -= xf->Volume->ClusterSize;
    }
    return ((xf->ClusterChain.at(Index) - 1) * xf->Volume->ClusterSize) + xf->Volume->DataStart + FilePosition;
}

BYTE xDeviceFileStream::ReadByte( void )
{
    if (IsClosed)
    {
        throw xException("Stream is closed. At: xDeviceFileStream::ReadByte");
    }
    else if (Position() > Length() - sizeof(BYTE))
    {
        throw xException("End of file reached.  At: xDeviceFileStream::ReadByte");
    }

    BYTE Return;
    ReadBytes(&Return, 0, 1);

    return Return;
}

short xDeviceFileStream::ReadInt16( void )
{
    if (IsClosed)
    {
        throw xException("Stream is closed. At: xDeviceFileStream::ReadInt16");
    }
    else if (Position() > Length() - sizeof(INT16))
    {
        throw xException("End of file reached.  At: xDeviceFileStream::ReadInt16");
    }

    BYTE temp[2];
    ReadBytes((BYTE*)&temp, 0, 2);

    short Return = (short)temp[0] << 8 | temp[1];

    return Return;
}

int xDeviceFileStream::ReadInt32( void )
{
    if (IsClosed)
    {
        throw xException("Stream is closed. At: xDeviceFileStream::ReadInt32");
    }
    else if (Position() > Length() - sizeof(INT32))
    {
        throw xException("End of file reached.  At: xDeviceFileStream::ReadInt32");
    }

    // Get the size of the object
    int size = sizeof(int);

    // Create a new temporary buffer that will hold the bytes for the object
    BYTE temp[4];
    // Read the data
    ReadBytes((BYTE*)&temp, 0, size);

    // Initialize our return value
    int Return = 0;

    /* Do a loop that starts out at size * 8 (for an Int32 that would be 24),
             * shift the data at index i by that value, OR that value.  Repeat after addition, subtraction and shit */
    for (int i = 0, j = (size * 8) - 8; i < size; i++, j -= 8)
    {
        Return |= (int)temp[i] << j;
    }

    return Return;
}

INT64 xDeviceFileStream::ReadInt64( void )
{
    if (IsClosed)
    {
        throw xException("Stream is closed. At: xDeviceFileStream::ReadInt64");
    }
    else if (Position() > Length() - sizeof(INT64))
    {
        throw xException("End of file reached.  At: xDeviceFileStream::ReadInt64");
    }

    int size = sizeof(INT64);

    BYTE temp[8];
    ReadBytes((BYTE*)&temp, 0, size);

    INT64 Return = 0;

    for (int i = 0, j = (size * 8) - 8; i < size; i++, j -= 8)
    {
        Return |= (INT64)temp[i] << j;
    }

    return Return;
}

UINT16 xDeviceFileStream::ReadUInt16( void )
{
    if (IsClosed)
    {
        throw xException("Stream is closed. At: xDeviceFileStream::ReadUInt16");
    }
    else if (Position() > Length() - sizeof(UINT16))
    {
        throw xException("End of file reached.  At: xDeviceFileStream::ReadUInt16");
    }

    int size = sizeof(UINT16);

    BYTE temp[2];
    ReadBytes((BYTE*)&temp, 0, size);

    UINT16 Return = 0;

    for (int i = 0, j = (size * 8) - 8; i < size; i++, j -= 8)
    {
        Return |= (UINT16)temp[i] << j;
    }

    return Return;
}

UINT32 xDeviceFileStream::ReadUInt32( void )
{
    if (IsClosed)
    {
        throw xException("Stream is closed. At: xDeviceFileStream::ReadUInt32");
    }
    else if (Position() > Length() - sizeof(UINT32))
    {
        throw xException("End of file reached.  At: xDeviceFileStream::ReadUInt32");
    }

    int size = sizeof(UINT32);

    BYTE temp[4];

    ReadBytes((BYTE*)&temp, 0, size);

    UINT32 Return = 0;

    for (int i = 0, j = (size * 8) - 8; i < size; i++, j -= 8)
    {
        Return |= (UINT32)temp[i] << j;
    }

    return Return;
}

UINT64 xDeviceFileStream::ReadUInt64( void )
{
    if (IsClosed)
    {
        throw xException("Stream is closed. At: xDeviceFileStream::ReadUInt64");
    }
    else if (Position() > Length() - sizeof(UINT64))
    {
        throw xException("End of file reached.  At: xDeviceFileStream::ReadUInt64");
    }

    int size = sizeof(UINT64);

    BYTE temp[8];
    ReadBytes((BYTE*)&temp, 0, size);

    UINT64 Return = 0;

    for (int i = 0, j = (size * 8) - 8; i < size; i++, j -= 8)
    {
        Return |= (UINT64)temp[i] << j;
    }

    return Return;
}

int xDeviceFileStream::ReadBytes( BYTE* DestBuff, int Offset, int Count )
{
    //SetPosition(Position());
    // How many clusters the data we're reading is spread across
    int ClustersSpanned = (Count + (UserPosition - Helpers::DownToNearestX(UserPosition, xf->Volume->ClusterSize))) / xf->Volume->ClusterSize;
    if (ClustersSpanned == 0)
        ++ClustersSpanned;
    // Read the first amount of data...
    int DataReadableInFirstCluster = xf->Volume->ClusterSize - (UserPosition - Helpers::DownToNearestX(UserPosition, (INT64)xf->Volume->ClusterSize));
    if (DataReadableInFirstCluster < Count)
    {
        int o = 0;
        int c = 0;
        c += device->DeviceStream->ReadBytes(DestBuff, 0, DataReadableInFirstCluster);
        o += c;
        SetPosition(Position() + c);
        Count -= c;
        for (int i = 1; i < ClustersSpanned - 1; i++)
        {
            c = device->DeviceStream->ReadBytes(DestBuff + o, 0, Count);
            o += c;
            Count -= c;
            SetPosition(Position() + c);
        }
        // Read the last bit of data
        c = device->DeviceStream->ReadBytes(DestBuff + o, 0, Count);
        SetPosition(Position() + c);
        o += c;
        return o;
    }
    else
    {
        int c = device->DeviceStream->ReadBytes(DestBuff, 0, Count);
        SetPosition(Position() + Count);
        return c;
    }
}

string xDeviceFileStream::ReadString( size_t Count )
{
    if (IsClosed)
    {
        throw xException("Stream is closed. At: xDeviceFileStream::ReadString");
    }
    if ((Position() + Count) > Length())
    {
        throw xException("Can not read beyond end of stream.  At: xDeviceFileStream::ReadString");
    }
    BYTE* Buffer = new BYTE[Count + 1];
    memset(Buffer, 0, Count + 1);

    ReadBytes(Buffer, 0, Count);
    string ret((char*)Buffer);
    delete[] Buffer;

    return ret;
}

string xDeviceFileStream::ReadCString( void )
{
    if (IsClosed)
    {
        throw xException("Stream is closed. At: xDeviceFileStream::ReadCString");
    }
    vector<char> temp;
    bool Null;
    do
    {
        char c = (char)ReadByte();
        temp.push_back(c);
        if (c == 0)
        {
            Null = true;
        }
    }
    while (!Null);

    char* tempString = new char[temp.size()];
    for (int i = 0; i < (int)temp.size(); i++)
    {
        tempString[i] = temp.at(i);
    }

    DetermineAndDoEndianSwap(tempString, temp.size() - 1, sizeof(char));

    string Return(tempString);

    delete[] tempString;
    return Return;
}

wstring xDeviceFileStream::ReadUnicodeString( size_t Count )
{
    if (IsClosed)
    {
        throw xException("Stream is closed. At: xDeviceFileStream::ReadUnicodeString");
    }
    else if ((Position() + Count) > Length())
    {
        throw xException("Can not read beyond end of stream.  At: xDeviceFileStream::ReadUnicodeString");
    }

    BYTE* Buffer = new BYTE[Count + 1];
    memset(Buffer, 0, Count + 1);

    ReadBytes(Buffer, 0, Count);
    for (int i = 0; i < Count; i += sizeof(wchar_t))
    {
        DetermineAndDoEndianSwap(Buffer + i, sizeof(wchar_t), sizeof(char));
    }
    wstring ret((TCHAR*)Buffer);
    delete[] Buffer;

    return ret;
}

void xDeviceFileStream::WriteByte( BYTE _Byte )
{
    if (IsClosed)
    {
        throw xException("Stream is closed. At: xDeviceFileStream::WriteByte");
    }
    else if (Position() + sizeof(BYTE) > Length())
    {
        throw xException("End of file reached.  At: xDeviceFileStream::WriteByte");
    }
    Write(&_Byte, 0, 1);
}

void xDeviceFileStream::WriteInt16( short _Int16 )
{
    if (IsClosed)
    {
        throw xException("Stream is closed. At: xDeviceFileStream::WriteInt16");
    }
    else if ((Position() + sizeof(short)) > Length())
    {
        throw xException("End of file reached.  At: xDeviceFileStream::WriteInt16");
    }
    DetermineAndDoEndianSwap((BYTE*)&_Int16, sizeof(short), sizeof(BYTE));
    Write((BYTE*)&_Int16, 0, sizeof(short));
}

void xDeviceFileStream::WriteInt32( int _Int32 )
{
    if (IsClosed)
    {
        throw xException("Stream is closed. At: xDeviceFileStream::WriteInt32");
    }
    else if ((Position() + sizeof(int)) > Length())
    {
        throw xException("End of file reached.  At: xDeviceFileStream::WriteInt16");
    }

    DetermineAndDoEndianSwap((BYTE*)&_Int32, sizeof(int), sizeof(BYTE));
    Write((BYTE*)&_Int32, 0, sizeof(int));
}

void xDeviceFileStream::WriteInt64( INT64 _Int64 )
{
    if (IsClosed)
    {
        throw xException("Stream is closed. At: xDeviceFileStream::WriteInt64");
    }
    else if ((Position() + sizeof(INT64)) > Length())
    {
        throw xException("End of file reached.  At: xDeviceFileStream::WriteInt64");
    }

    DetermineAndDoEndianSwap((BYTE*)&_Int64, sizeof(INT64), sizeof(BYTE));
    Write((BYTE*)&_Int64, 0, sizeof(INT64));
}

void xDeviceFileStream::WriteUInt16( UINT16 _UInt16 )
{
    if (IsClosed)
    {
        throw xException("Stream is closed. At: xDeviceFileStream::WriteUInt16");
    }
    else if ((Position() + sizeof(UINT16)) > Length())
    {
        throw xException("End of file reached.  At: xDeviceFileStream::WriteUInt16");
    }

    DetermineAndDoEndianSwap((BYTE*)&_UInt16, sizeof(UINT16), sizeof(BYTE));
    Write((BYTE*)&_UInt16, 0, sizeof(UINT16));
}

void xDeviceFileStream::WriteUInt32( UINT32 _UInt32 )
{
    if (IsClosed)
    {
        throw xException("Stream is closed. At: xDeviceFileStream::WriteUInt32");
    }
    else if ((Position() + sizeof(UINT32)) > Length())
    {
        throw xException("End of file reached.  At: xDeviceFileStream::WriteUInt32");
    }

    DetermineAndDoEndianSwap((BYTE*)&_UInt32, sizeof(UINT32), sizeof(BYTE));
    Write((BYTE*)&_UInt32, 0, sizeof(UINT32));
}

void xDeviceFileStream::WriteUInt64( UINT64 _UInt64 )
{
    if (IsClosed)
    {
        throw xException("Stream is closed. At: xDeviceFileStream::WriteUInt64");
    }
    else if ((Position() + sizeof(UINT64)) > Length())
    {
        throw xException("End of file reached.  At: xDeviceFileStream::WriteUInt64");
    }

    DetermineAndDoEndianSwap((BYTE*)&_UInt64, sizeof(UINT64), sizeof(BYTE));
    Write((BYTE*)&_UInt64, 0, sizeof(UINT64));
}

int xDeviceFileStream::Write( BYTE* Buffer, int offset, int count )
{
#warning "ADD HANDING FOR EXTENDING FILES"
    //SetPosition(Position());
    // How many clusters the data we're writing is spread across
    int ClustersSpanned = (count + (UserPosition - Helpers::DownToNearestX(UserPosition, xf->Volume->ClusterSize))) / xf->Volume->ClusterSize;
    // Read the first amount of data...
    int DataWritableInFirstCluster = xf->Volume->ClusterSize - (UserPosition - Helpers::DownToNearestX(UserPosition, (INT64)xf->Volume->ClusterSize));
    if (DataWritableInFirstCluster < count)
    {
        int o = 0;
        int c = 0;
        c += device->DeviceStream->ReadBytes(Buffer, 0, DataWritableInFirstCluster);
        o += c;
        SetPosition(Position() + c);
        count -= c;
        for (int i = 1; i < ClustersSpanned - 1; i++)
        {
            c = device->DeviceStream->ReadBytes(Buffer + o, 0, count);
            o += c;
            count -= c;
            SetPosition(Position() + c);
        }
        // Read the last bit of data
        c = device->DeviceStream->ReadBytes(Buffer + o, 0, count);
        SetPosition(Position() + c);
        o += c;
        return o;
    }
    else
    {
        int c = device->DeviceStream->ReadBytes(Buffer, 0, count);
        SetPosition(Position() + c);
        return c;
    }
}

}
