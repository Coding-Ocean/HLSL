class File;

class FILE_BUFFER{
public:
    FILE_BUFFER();
    void operator= ( const char* fileName );
    ~FILE_BUFFER();

    //バッファポインタを進める系
    const char* readString();
    const char* readChar();
    void readOnAssumption( char* s );
    float readFloat();
    int readInt();
    unsigned readUnsigned();
    void skipNode();
    void restart();
    bool end();

    void setBuffer( const char* data, int size );
    //取得系
    const char* buffer() const;
    int size();
    const char* bufferPointer() const;
    const char* string() const;
    operator const char*();
    //オーバーロード
    bool operator==( const char* str );
    bool operator!=( const char* str );
    bool operator==( char c );
    bool operator!=( char c );
private:
    char* Buffer;//ファイルバッファ
    int Size;//バッファサイズ
    const char* BufferPointer;//ファイルバッファのポインタ
    char String[ 256 ];//取り出した文字列
    char Character[ 3 ];
    bool SetBufferFlag;
};


