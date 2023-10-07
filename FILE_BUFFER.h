class File;

class FILE_BUFFER{
public:
    FILE_BUFFER();
    void operator= ( const char* fileName );
    ~FILE_BUFFER();

    //�o�b�t�@�|�C���^��i�߂�n
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
    //�擾�n
    const char* buffer() const;
    int size();
    const char* bufferPointer() const;
    const char* string() const;
    operator const char*();
    //�I�[�o�[���[�h
    bool operator==( const char* str );
    bool operator!=( const char* str );
    bool operator==( char c );
    bool operator!=( char c );
private:
    char* Buffer;//�t�@�C���o�b�t�@
    int Size;//�o�b�t�@�T�C�Y
    const char* BufferPointer;//�t�@�C���o�b�t�@�̃|�C���^
    char String[ 256 ];//���o����������
    char Character[ 3 ];
    bool SetBufferFlag;
};


