
#include <windows.h>
#include <ksguid.h>
#include <stdio.h>
#include <iostream>
#include <fstream>
#include <vector>

#define _Analysis_mode_(...)
#define _Notliteral_
#define __user_driver
#define _Out_writes_bytes_opt_
#define _In_reads_bytes_opt_(...)
#define _Out_writes_bytes_opt_(...)

#include <wudfddi.h>

#define NTDDI_VERSION NTDDI_WIN7
#include "winbio_ioctl.h"

#include "breakpoints.h"

extern "C" {
HRESULT WINAPI PropVariantToInt32(REFPROPVARIANT propvarIn, LONG *ret);

int WINAPI PropVariantToStringAlloc(
  REFPROPVARIANT propvar,
  PWSTR          *ppszOut
);
}

typedef WINAPI DllGetClassObject_t(_In_ REFCLSID rclsid, _In_ REFIID riid, _Out_ LPVOID* ppv);
//WINAPI DllGetClassObject(_In_ REFCLSID rclsid, _In_ REFIID riid, _Out_ LPVOID* ppv);

// 96710705-b080-4b29-a3ec-b16935ae663a
DEFINE_GUID(SYNA_CLSID, 0x96710705, 0xb080, 0x4b29, 0xa3, 0xec, 0xb1, 0x69, 0x35, 0xae, 0x66, 0x3a);

// 1BEC7499-8881-4F2B-B01C-A1A907304AFC
DEFINE_GUID(IID_IDriverEntry, 0x1BEC7499, 0x8881, 0x4F2B, 0xB0, 0x1C, 0xA1, 0xA9, 0x07, 0x30, 0x4A, 0xFC);

DEFINE_GUID(IID_IQueueCallbackDeviceIoControl, 0xC5411408, 0x0F1E, 0x4ed6, 0xA4, 0x12, 0x36, 0xDD, 0x15, 0xEE, 0xE7, 0x07);

DEFINE_GUID(IID_IPnpCallbackHardware, 0x51433BD3, 0xC7C1, 0x4bd8, 0xB4, 0xC1, 0xAB, 0x1E, 0x03, 0x46, 0x26, 0xCC);

DEFINE_GUID(IID_IPnpCallback, 0x27c32374, 0xcc45, 0x4840, 0x85, 0x7e, 0x8e, 0x5e, 0xf7, 0xc0, 0xeb, 0xff);

DEFINE_GUID(IID_IWDFPropertyStoreFactory, 0x45BE7E06, 0x9B65, 0x434d, 0xA7, 0xD6, 0x95, 0x72, 0xD7, 0xF7, 0x3D, 0x53);

int goIdle;

struct MyNamedPropertyStore : public IWDFNamedPropertyStore2 {
    public:
        virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void **ppvObject) { 
            LPOLESTR str;
            StringFromIID(riid, &str);
            std::wcout << L"MyMem::QueryInterface " << str << std::endl;
            *ppvObject = this;
            printf("ppvObject=%p\r\n", *ppvObject);
            return 0; 
        }
        virtual ULONG STDMETHODCALLTYPE AddRef(void) { 
            printf("AddRef\r\n");
            return 0; 
        }
        virtual ULONG STDMETHODCALLTYPE Release(void) {
            //printf("MyMem::Release\r\n");
            return 0;
        }
    public:

        virtual HRESULT STDMETHODCALLTYPE DeleteWdfObject( void) {
            printf("DeleteWdfObject\r\n");
            return 0; 
        }
        
        virtual HRESULT STDMETHODCALLTYPE AssignContext( 
            /* [annotation][unique][in] */ 
            _In_opt_ __drv_aliasesMem  IObjectCleanup *pCleanupCallback,
            /* [annotation][unique][in] */ 
            _In_opt_ __drv_aliasesMem  void *pContext) { 
            printf("AssignContext\r\n");
            return 0;
        }
        
        virtual HRESULT STDMETHODCALLTYPE RetrieveContext( 
            /* [annotation][out] */ 
            _Out_  void **ppvContext) {
            printf("RetrieveContext\r\n");
            return 0;
        }
        
        virtual void STDMETHODCALLTYPE AcquireLock( void) {
            printf("AcquireLock\r\n");
        }
        
        virtual void STDMETHODCALLTYPE ReleaseLock( void) {
            printf("ReleaseLock\r\n");
        }
    public:
        virtual HRESULT STDMETHODCALLTYPE GetNamedValue( 
            /* [annotation][string][in] */ 
            _In_  LPCWSTR pszName,
            /* [annotation][out] */ 
            _Out_  PROPVARIANT *pv){
            std::wcout << L"=====================================" << std::endl;
            std::wcout << L"GetNamedValue " << pszName << std::endl;
            std::wcout << L"=====================================" << std::endl;

            std::string fname;
            std::wstring wfname(pszName);
            fname.assign(wfname.begin(), wfname.end());
            if(fname == "CalibrationData") {
                std::ifstream input(fname + ".blob", std::ios::binary);
                std::vector<char> buf((std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>());
                std::cout << "Loaded " << buf.size() << " bytes of calibration data" << std::endl;
                if(buf.size() == 0) {
                    DECIMAL_SETZERO(pv->decVal);
                } else {
                    pv->vt = VT_BLOB;
                    pv->blob.cbSize = buf.size();
                    pv->blob.pBlobData = (BYTE*)CoTaskMemAlloc(pv->blob.cbSize);
                    std::copy(buf.begin(), buf.end(), pv->blob.pBlobData);
                }
            } 
            else if(fname == "LastUpdateSystemTimeStamp" || fname == "OldCalDataDeleted") {
                std::ifstream input(fname + ".uint");
                if(input) {
                    pv->vt = VT_UINT;
                    input >> pv->uintVal;
                    std::cout << "UINT " << fname << " = " << pv->uintVal << std::endl; 
                }
                else {
                    DECIMAL_SETZERO(pv->decVal);
                    std::cout << "UINT " << fname << " not found" << std::endl; 
                }
            } 
            else {
                DECIMAL_SETZERO(pv->decVal);
            }
            return 0;
        }

        
        virtual HRESULT STDMETHODCALLTYPE SetNamedValue( 
            /* [annotation][string][in] */ 
            _In_  LPCWSTR pszName,
            /* [annotation][in] */ 
            _In_  const PROPVARIANT *pv){
            //wchar_t *buf = L"<error>";
            //LONG num = 118;
            //PropVariantToInt32(*pv, &num);
            //PropVariantToStringAlloc(*pv, &buf);

            std::wcout << L"=====================================" << std::endl;
            std::wcout << L"SetNamedValue " << pszName << "=" << pv->vt << std::endl;
            switch(pv->vt) {
                case VT_I1:
                    printf("VT_I1: %d\n", pv->cVal);
                    break;
                case VT_UINT: {
                        std::string fname;
                        std::wstring wfname(pszName);
                        fname.assign(wfname.begin(), wfname.end());
                        std::ofstream output(fname + ".uint");
                        output << pv->uintVal << std::endl;
                        printf("VT_UINT: %d\n", pv->uintVal);
                    }
                    break;
                case VT_BLOB: {
                        std::string fname;
                        std::wstring wfname(pszName);
                        fname.assign(wfname.begin(), wfname.end());
                        std::ofstream output(fname + ".blob", std::ios::binary);
                        std::copy(pv->blob.pBlobData, pv->blob.pBlobData + pv->blob.cbSize,
                                std::ostreambuf_iterator<char>(output));

                        char hex[800020], *p = hex;
                        for(unsigned int i=0;i<(pv->blob.cbSize < 400? pv->blob.cbSize : 400);i++) {
                            p+=sprintf(p, "%02x", pv->blob.pBlobData[i]);
                        }
                        *p=0;
                        printf("Blob value: %lu: %s\n", pv->blob.cbSize, hex);
                    }
                    break;
            }
            std::wcout << L"=====================================" << std::endl;
            //CoTaskMemFree(buf);
            return 0;
        }

        
        virtual HRESULT STDMETHODCALLTYPE GetNameCount( 
            /* [annotation][out] */ 
            _Out_  DWORD *pdwCount){
            std::wcout << L"GetNameCount " << std::endl;
            *pdwCount = 0;
            return 0;
        }

        
        virtual HRESULT STDMETHODCALLTYPE GetNameAt( 
            /* [annotation][in] */ 
            _In_  DWORD iProp,
            /* [annotation][string][out] */ 
            _Out_  PWSTR *ppwszName){
            std::wcout << L"GetNameAt " << iProp << std::endl;
            *ppwszName = L"";
            return 0;
        }

    public:
        virtual HRESULT STDMETHODCALLTYPE DeleteNamedValue( 
            /* [annotation][string][in] */ 
            _In_  LPCWSTR pwszName){
            std::wcout << L"DeleteNamedValue " << pwszName<< std::endl;
            return 0;
        }

};


struct MyPropertyStoreFactory : public IWDFPropertyStoreFactory {
    public:
        virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void **ppvObject) { 
            LPOLESTR str;
            StringFromIID(riid, &str);
            std::wcout << L"MyPropertyStoreFactory::QueryInterface " << str << std::endl;
            *ppvObject = this;
            printf("ppvObject=%p\r\n", *ppvObject);
            return 0; 
        }
        virtual ULONG STDMETHODCALLTYPE AddRef(void) { 
            printf("AddRef\r\n");
            return 0; 
        }
        virtual ULONG STDMETHODCALLTYPE Release(void) {
            printf("MyPropertyStoreFactory::Release\r\n");
            return 0;
        }
    public:

        virtual HRESULT STDMETHODCALLTYPE DeleteWdfObject( void) {
            printf("DeleteWdfObject\r\n");
            return 0; 
        }
        
        virtual HRESULT STDMETHODCALLTYPE AssignContext( 
            /* [annotation][unique][in] */ 
            _In_opt_ __drv_aliasesMem  IObjectCleanup *pCleanupCallback,
            /* [annotation][unique][in] */ 
            _In_opt_ __drv_aliasesMem  void *pContext) { 
            printf("AssignContext\r\n");
            return 0;
        }
        
        virtual HRESULT STDMETHODCALLTYPE RetrieveContext( 
            /* [annotation][out] */ 
            _Out_  void **ppvContext) {
            printf("RetrieveContext\r\n");
            return 0;
        }
        
        virtual void STDMETHODCALLTYPE AcquireLock( void) {
            printf("AcquireLock\r\n");
        }
        
        virtual void STDMETHODCALLTYPE ReleaseLock( void) {
            printf("ReleaseLock\r\n");
        }
    public:
        virtual HRESULT STDMETHODCALLTYPE RetrieveDevicePropertyStore( 
            /* [annotation][in] */ 
            _In_  PWDF_PROPERTY_STORE_ROOT RootSpecifier,
            /* [annotation][in] */ 
            _In_  WDF_PROPERTY_STORE_RETRIEVE_FLAGS Flags,
            /* [annotation][in] */ 
            _In_  REGSAM DesiredAccess,
            /* [annotation][unique][in] */ 
            _In_opt_  PCWSTR SubkeyPath,
            /* [annotation][out] */ 
            _Out_  IWDFNamedPropertyStore2 **PropertyStore,
            /* [annotation][unique][out] */ 
            _Out_opt_  WDF_PROPERTY_STORE_DISPOSITION *Disposition){
            printf("RetrieveDevicePropertyStore\r\n");
            *PropertyStore = new MyNamedPropertyStore();
            return 0;
        }

};
        
struct MyMem : public IWDFMemory {
    public:
        MyMem(void *b, SIZE_T s) {
            buf = b;
            size = s;
        }

    public:
        virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void **ppvObject) { 
            LPOLESTR str;
            StringFromIID(riid, &str);
            std::wcout << L"MyMem::QueryInterface " << str << std::endl;
            *ppvObject = this;
            printf("ppvObject=%p\r\n", *ppvObject);
            return 0; 
        }
        virtual ULONG STDMETHODCALLTYPE AddRef(void) { 
            printf("AddRef\r\n");
            return 0; 
        }
        virtual ULONG STDMETHODCALLTYPE Release(void) {
            //printf("MyMem::Release\r\n");
            return 0;
        }
    public:

        virtual HRESULT STDMETHODCALLTYPE DeleteWdfObject( void) {
            printf("DeleteWdfObject\r\n");
            return 0; 
        }
        
        virtual HRESULT STDMETHODCALLTYPE AssignContext( 
            /* [annotation][unique][in] */ 
            _In_opt_ __drv_aliasesMem  IObjectCleanup *pCleanupCallback,
            /* [annotation][unique][in] */ 
            _In_opt_ __drv_aliasesMem  void *pContext) { 
            printf("AssignContext\r\n");
            return 0;
        }
        
        virtual HRESULT STDMETHODCALLTYPE RetrieveContext( 
            /* [annotation][out] */ 
            _Out_  void **ppvContext) {
            printf("RetrieveContext\r\n");
            return 0;
        }
        
        virtual void STDMETHODCALLTYPE AcquireLock( void) {
            printf("AcquireLock\r\n");
        }
        
        virtual void STDMETHODCALLTYPE ReleaseLock( void) {
            printf("ReleaseLock\r\n");
        }
    public:
        virtual HRESULT STDMETHODCALLTYPE CopyFromMemory( 
            /* [annotation][in] */ 
            _In_  IWDFMemory *Source,
            /* [annotation][unique][in] */ 
            _In_opt_  PWDFMEMORY_OFFSET SourceOffset){
            printf("CopyFromMemory\r\n");
            return 0;
        }

        
        virtual HRESULT STDMETHODCALLTYPE CopyToBuffer( 
            /* [annotation][in] */ 
            _In_  ULONG_PTR SourceOffset,
            /* [annotation][size_is][in] */ 
            _Out_writes_bytes_(NumOfBytesToCopyTo)  void *TargetBuffer,
            /* [annotation][in] */ 
            _In_  SIZE_T NumOfBytesToCopyTo){
            printf("CopyToBuffer\r\n");
            return 0;
        }

        
        virtual HRESULT STDMETHODCALLTYPE CopyFromBuffer( 
            /* [annotation][in] */ 
            _In_  ULONG_PTR DestOffset,
            /* [annotation][size_is][in] */ 
            _In_reads_bytes_(NumOfBytesToCopyFrom)  void *SourceBuffer,
            /* [annotation][in] */ 
            _In_  SIZE_T NumOfBytesToCopyFrom){
            printf("CopyFromBuffer\r\n");
            return 0;
        }

        
        virtual SIZE_T STDMETHODCALLTYPE GetSize( void){
            printf("GetSize\r\n");
            return size;
        }

        
        virtual void *STDMETHODCALLTYPE GetDataBuffer( 
            /* [annotation][unique][out] */ 
            _Out_opt_  SIZE_T *BufferSize){
            if(BufferSize) {
                *BufferSize = size;
            }
            // Sleep(5000);
            return buf;
        }

        
        virtual void STDMETHODCALLTYPE SetBuffer( 
            /* [annotation][size_is][in] */ 
            _In_reads_bytes_(BufferSize)  void *Buffer,
            /* [annotation][in] */ 
            _In_  SIZE_T BufferSize){
            printf("SetBuffer\r\n");
            buf = Buffer;
            size = BufferSize;
        }

    void * buf;
    SIZE_T size;
};

struct MyRequest : public IWDFIoRequest {
    public:
        MyRequest(WDF_REQUEST_TYPE t, ULONG c, MyMem *out, MyMem *in) {
            reqType = t;
            ctl = c;
            outMem = out;
            inMem = in;
            complete = FALSE;
            informationSize = 0;
        }

        WDF_REQUEST_TYPE reqType;
        ULONG ctl;
        BOOL complete;
        LONG_PTR informationSize;
    public:
        virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void **ppvObject) { 
            LPOLESTR str;
            StringFromIID(riid, &str);
            std::wcout << L"MyRequest::QueryInterface " << str << std::endl;
            *ppvObject = this;
            printf("ppvObject=%p\r\n", *ppvObject);
            return 0; 
        }
        virtual ULONG STDMETHODCALLTYPE AddRef(void) { 
            printf("AddRef\r\n");
            return 0; 
        }
        virtual ULONG STDMETHODCALLTYPE Release(void) {
            printf("MyRequest::Release\r\n");
            return 0;
        }
    public:

        virtual HRESULT STDMETHODCALLTYPE DeleteWdfObject( void) {
            printf("DeleteWdfObject\r\n");
            return 0; 
        }
        
        virtual HRESULT STDMETHODCALLTYPE AssignContext( 
            /* [annotation][unique][in] */ 
            _In_opt_ __drv_aliasesMem  IObjectCleanup *pCleanupCallback,
            /* [annotation][unique][in] */ 
            _In_opt_ __drv_aliasesMem  void *pContext) { 
            printf("AssignContext\r\n");
            return 0;
        }
        
        virtual HRESULT STDMETHODCALLTYPE RetrieveContext( 
            /* [annotation][out] */ 
            _Out_  void **ppvContext) {
            printf("RetrieveContext\r\n");
            return 0;
        }
        
        virtual void STDMETHODCALLTYPE AcquireLock( void) {
            printf("AcquireLock\r\n");
        }
        
        virtual void STDMETHODCALLTYPE ReleaseLock( void) {
            printf("ReleaseLock\r\n");
        }
    public:
        virtual void STDMETHODCALLTYPE CompleteWithInformation( 
            /* [annotation][in] */ 
            _In_  HRESULT CompletionStatus,
            /* [annotation][in] */ 
            _In_  SIZE_T Information){
            printf("CompleteWithInformation\r\n");
            complete = TRUE;
        }

        
        virtual void STDMETHODCALLTYPE SetInformation( 
            /* [annotation][in] */ 
            _In_  ULONG_PTR Information){
            printf("SetInformation size=%lld\r\n", Information);
            informationSize = Information;
        }

        
        virtual void STDMETHODCALLTYPE Complete( 
            /* [annotation][in] */ 
            _In_  HRESULT CompletionStatus){
            printf("Complete: %lx\r\n", (unsigned long)CompletionStatus);
            complete = TRUE;
        }

        
        virtual void STDMETHODCALLTYPE SetCompletionCallback( 
            /* [annotation][in] */ 
            _In_  IRequestCallbackRequestCompletion *pCompletionCallback,
            /* [annotation][unique][in] */ 
            _In_opt_  void *pContext){
            printf("SetCompletionCallback\r\n");
        }

        
        virtual WDF_REQUEST_TYPE STDMETHODCALLTYPE GetType( void){
            printf("GetType\r\n");
            return reqType;
        }

        
        virtual void STDMETHODCALLTYPE GetCreateParameters( 
            /* [annotation][unique][out] */ 
            _Out_opt_  ULONG *pOptions,
            /* [annotation][unique][out] */ 
            _Out_opt_  USHORT *pFileAttributes,
            /* [annotation][unique][out] */ 
            _Out_opt_  USHORT *pShareAccess){
            printf("GetCreateParameters\r\n");
        }

        
        virtual void STDMETHODCALLTYPE GetReadParameters( 
            /* [annotation][unique][out] */ 
            _Out_opt_  SIZE_T *pSizeInBytes,
            /* [annotation][unique][out] */ 
            _Out_opt_  LONGLONG *pullOffset,
            /* [annotation][unique][out] */ 
            _Out_opt_  ULONG *pulKey){
            printf("GetReadParameters\r\n");
        }

        
        virtual void STDMETHODCALLTYPE GetWriteParameters( 
            /* [annotation][unique][out] */ 
            _Out_opt_  SIZE_T *pSizeInBytes,
            /* [annotation][unique][out] */ 
            _Out_opt_  LONGLONG *pullOffset,
            /* [annotation][unique][out] */ 
            _Out_opt_  ULONG *pulKey){
            printf("GetWriteParameters\r\n");
        }

        
        virtual void STDMETHODCALLTYPE GetDeviceIoControlParameters( 
            /* [annotation][unique][out] */ 
            _Out_opt_  ULONG *pControlCode,
            /* [annotation][unique][out] */ 
            _Out_opt_  SIZE_T *pInBufferSize,
            /* [annotation][unique][out] */ 
            _Out_opt_  SIZE_T *pOutBufferSize){
            //printf("GetDeviceIoControlParameters %p %p %p\r\n", pControlCode, pInBufferSize, pOutBufferSize);
            *pControlCode = ctl;
            *pInBufferSize = inMem->size;
            *pOutBufferSize = outMem->size;
        }

        
        MyMem *outMem, *inMem;

        virtual void STDMETHODCALLTYPE GetOutputMemory( 
            /* [annotation][out] */ 
            _Out_  IWDFMemory **ppWdfMemory){
            *ppWdfMemory = outMem;
            //printf("GetOutputMemory\r\n");
        }
        
        virtual void STDMETHODCALLTYPE GetInputMemory( 
            /* [annotation][out] */ 
            _Out_  IWDFMemory **ppWdfMemory){
            *ppWdfMemory = inMem;
            // printf("GetInputMemory\r\n");
        }

        
        virtual void STDMETHODCALLTYPE MarkCancelable( 
            /* [annotation][in] */ 
            _In_  IRequestCallbackCancel *pCancelCallback){
            printf("MarkCancelable\r\n");
            //this->cancelCallback = pCancelCallback;
        }

        virtual HRESULT STDMETHODCALLTYPE UnmarkCancelable( void){
            printf("UnmarkCancelable\r\n");
            return 0;
        }

        
        virtual BOOL STDMETHODCALLTYPE CancelSentRequest( void){
            printf("CancelSentRequest\r\n");
            return 0;
        }

        
        virtual HRESULT STDMETHODCALLTYPE ForwardToIoQueue( 
            /* [annotation][in] */ 
            _In_  IWDFIoQueue *pDestination){
            printf("ForwardToIoQueue\r\n");
            return 0;
        }

        
        virtual HRESULT STDMETHODCALLTYPE Send( 
            /* [annotation][in] */ 
            _In_  IWDFIoTarget *pIoTarget,
            /* [annotation][in] */ 
            _In_  ULONG Flags,
            /* [annotation][in] */ 
            _In_  LONGLONG Timeout){
            printf("Send\r\n");
            return 0;
        }

        
        virtual void STDMETHODCALLTYPE GetFileObject( 
            /* [annotation][out] */ 
            _Out_  IWDFFile **ppFileObject){
            printf("GetFileObject\r\n");
        }

        
        virtual void STDMETHODCALLTYPE FormatUsingCurrentType( void){
            printf("FormatUsingCurrentType\r\n");
        }

        
        virtual ULONG STDMETHODCALLTYPE GetRequestorProcessId( void){
            printf("GetRequestorProcessId\r\n");
            return 0;
        }

        
        virtual void STDMETHODCALLTYPE GetIoQueue( 
            /* [annotation][out] */ 
            _Out_  IWDFIoQueue **ppWdfIoQueue){
            printf("GetIoQueue\r\n");
        }

        
        virtual HRESULT STDMETHODCALLTYPE Impersonate( 
            /* [annotation][in] */ 
            _In_  SECURITY_IMPERSONATION_LEVEL ImpersonationLevel,
            /* [annotation][in] */ 
            _In_  IImpersonateCallback *pCallback,
            /* [annotation][unique][in] */ 
            _In_opt_  void *pvCallbackContext){
            printf("Impersonate\r\n");
            return 0;
        }

        
        virtual BOOL STDMETHODCALLTYPE IsFrom32BitProcess( void){
            printf("IsFrom32BitProcess\r\n");
            return 1;
        }

        
        virtual void STDMETHODCALLTYPE GetCompletionParams( 
            /* [annotation][out] */ 
            _Out_  IWDFRequestCompletionParams **ppCompletionParams){
            printf("GetCompletionParams\r\n");
        }

        
};

struct MyQueue : public IWDFIoQueue {
    public:
        MyQueue(IUnknown *pCallbackInterface) {
            pCallbackInterface->AddRef();
            pCallbackInterface->QueryInterface(IID_IQueueCallbackDeviceIoControl, (LPVOID*)&ioctl);
            printf("ioctl=%p\r\n", ioctl);
        }

        IQueueCallbackDeviceIoControl *ioctl=0;

    public:
        virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void **ppvObject) { 
            LPOLESTR str;
            StringFromIID(riid, &str);
            std::wcout << L"MyQueue::QueryInterface " << str << std::endl;
            *ppvObject = this;
            printf("ppvObject=%p\r\n", *ppvObject);
            return 0; 
        }
        virtual ULONG STDMETHODCALLTYPE AddRef(void) { 
            printf("AddRef\r\n");
            return 0; 
        }
        virtual ULONG STDMETHODCALLTYPE Release(void) {
            printf("MyQueue::Release\r\n");
            return 0;
        }
    public:

        virtual HRESULT STDMETHODCALLTYPE DeleteWdfObject( void) {
            printf("DeleteWdfObject\r\n");
            return 0; 
        }
        
        virtual HRESULT STDMETHODCALLTYPE AssignContext( 
            /* [annotation][unique][in] */ 
            _In_opt_ __drv_aliasesMem  IObjectCleanup *pCleanupCallback,
            /* [annotation][unique][in] */ 
            _In_opt_ __drv_aliasesMem  void *pContext) { 
            printf("AssignContext\r\n");
            return 0;
        }
        
        virtual HRESULT STDMETHODCALLTYPE RetrieveContext( 
            /* [annotation][out] */ 
            _Out_  void **ppvContext) {
            printf("RetrieveContext\r\n");
            return 0;
        }
        
        virtual void STDMETHODCALLTYPE AcquireLock( void) {
            printf("AcquireLock\r\n");
        }
        
        virtual void STDMETHODCALLTYPE ReleaseLock( void) {
            printf("ReleaseLock\r\n");
        }
    public:
        virtual void STDMETHODCALLTYPE GetDevice( 
            /* [annotation][out] */ 
            _Out_  IWDFDevice **ppWdfDevice){
            printf("GetDevice\r\n");
        }

        
        virtual HRESULT STDMETHODCALLTYPE ConfigureRequestDispatching( 
            /* [annotation][in] */ 
            _In_  WDF_REQUEST_TYPE RequestType,
            /* [annotation][in] */ 
            _In_  BOOL Forward){
            printf("MyQueue::ConfigureRequestDispatching\r\n");
            return 0;
        }

        
        virtual WDF_IO_QUEUE_STATE STDMETHODCALLTYPE GetState( 
            /* [annotation][out] */ 
            _Out_  ULONG *pulNumOfRequestsInQueue,
            /* [annotation][out] */ 
            _Out_  ULONG *pulNumOfRequestsInDriver){
            printf("GetState\r\n");
            return (WDF_IO_QUEUE_STATE)(WdfIoQueueAcceptRequests | 
                    WdfIoQueueDispatchRequests | 
                    WdfIoQueueNoRequests | 
                    WdfIoQueueDriverNoRequests);
        }

        
        virtual HRESULT STDMETHODCALLTYPE RetrieveNextRequest( 
            /* [annotation][out] */ 
            _Out_  IWDFIoRequest **ppRequest){
            printf("RetrieveNextRequest\r\n");
            return 0;
        }

        
        virtual HRESULT STDMETHODCALLTYPE RetrieveNextRequestByFileObject( 
            /* [annotation][in] */ 
            _In_  IWDFFile *pFile,
            /* [annotation][out] */ 
            _Out_  IWDFIoRequest **ppRequest){
            printf("RetrieveNextRequestByFileObject\r\n");
            return 0;
        }

        
        virtual void STDMETHODCALLTYPE Start( void){
            printf("Start\r\n");
        }

        
        virtual void STDMETHODCALLTYPE Stop( 
            /* [annotation][unique][in] */ 
            _In_opt_  IQueueCallbackStateChange *pStopComplete){
            printf("Stop\r\n");
        }

        
        virtual void STDMETHODCALLTYPE StopSynchronously( void){
            printf("StopSynchronously\r\n");
        }

        
        virtual void STDMETHODCALLTYPE Drain( 
            /* [annotation][unique][in] */ 
            _In_opt_  IQueueCallbackStateChange *pDrainComplete){
            printf("Drain\r\n");
        }

        
        virtual void STDMETHODCALLTYPE DrainSynchronously( void){
            printf("Drain\r\n");
        }

        
        virtual void STDMETHODCALLTYPE Purge( 
            /* [annotation][unique][in] */ 
            _In_opt_  IQueueCallbackStateChange *pPurgeComplete){
            printf("Purge\r\n");
        }

        
        virtual void STDMETHODCALLTYPE PurgeSynchronously( void){
            printf("Purge\r\n");
        }


};


MyQueue *myQueue = 0;

struct MyDevice;

MyDevice *myDevice = 0;


struct MyDevice : public IWDFDevice3 {
    public:
        MyDevice(IUnknown *pCallbackInterface) {
            pCallbackInterface->AddRef();
            pCallbackInterface->QueryInterface(IID_IPnpCallbackHardware, (LPVOID*)&pnphwcb);
            pCallbackInterface->QueryInterface(IID_IPnpCallback, (LPVOID*)&pnpcb);
            printf("pnphwcb=%p pnpcb=%p\r\n", pnphwcb, pnpcb);
        }

        IPnpCallbackHardware *pnphwcb;
        IPnpCallback *pnpcb;
    public:
        virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void **ppvObject) { 
            LPOLESTR str;
            StringFromIID(riid, &str);
            std::wcout << L"MyDevice::QueryInterface " << str << std::endl;

            if(IsEqualIID(riid, IID_IWDFPropertyStoreFactory)) {
                printf("is IID_IWDFPropertyStoreFactory\r\n");
                *ppvObject = new MyPropertyStoreFactory();
            } else {
                printf("is IID_IWDFDevice3\r\n");
                *ppvObject = (IWDFDevice3*)this;
            }
            printf("ppvObject=%p\r\n", *ppvObject);
            return 0; 
        }
        virtual ULONG STDMETHODCALLTYPE AddRef(void) { 
            printf("AddRef\r\n");
            return 0; 
        }
        virtual ULONG STDMETHODCALLTYPE Release(void) {
            printf("MyDevice::Release\r\n");
            return 0;
        }
    public:

        virtual HRESULT STDMETHODCALLTYPE DeleteWdfObject( void) {
            printf("DeleteWdfObject\r\n");
            return 0; 
        }
        
        virtual HRESULT STDMETHODCALLTYPE AssignContext( 
            /* [annotation][unique][in] */ 
            _In_opt_ __drv_aliasesMem  IObjectCleanup *pCleanupCallback,
            /* [annotation][unique][in] */ 
            _In_opt_ __drv_aliasesMem  void *pContext) { 
            printf("AssignContext\r\n");
            return 0;
        }
        
        virtual HRESULT STDMETHODCALLTYPE RetrieveContext( 
            /* [annotation][out] */ 
            _Out_  void **ppvContext) {
            printf("RetrieveContext\r\n");
            return 0;
        }
        
        virtual void STDMETHODCALLTYPE AcquireLock( void) {
            printf("AcquireLock\r\n");
        }
        
        virtual void STDMETHODCALLTYPE ReleaseLock( void) {
            printf("ReleaseLock\r\n");
        }

    public:
        virtual HRESULT STDMETHODCALLTYPE RetrieveDevicePropertyStore( 
            /* [annotation][unique][in] */ 
            _In_opt_  PCWSTR pcwszServiceName,
            /* [annotation][in] */ 
            _In_  WDF_PROPERTY_STORE_RETRIEVE_FLAGS Flags,
            /* [annotation][out] */ 
            _Out_  IWDFNamedPropertyStore **ppPropStore,
            /* [annotation][unique][out] */ 
            _Out_opt_  WDF_PROPERTY_STORE_DISPOSITION *pDisposition){
            printf("RetrieveDevicePropertyStore\r\n");
            *ppPropStore = new MyNamedPropertyStore();
            return 0;
        }

        
        virtual void STDMETHODCALLTYPE GetDriver( 
            /* [annotation][out] */ 
            _Out_  IWDFDriver **ppWdfDriver){
            printf("GetDriver\r\n");
        }

        
        virtual HRESULT STDMETHODCALLTYPE RetrieveDeviceInstanceId( 
            /* [annotation][unique][out][string] */ 
            _Out_opt_  PWSTR Buffer,
            /* [annotation][out][in] */ 
            _Inout_  DWORD *pdwSizeInChars){
            printf("RetrieveDeviceInstanceId\r\n");
            return 0;
        }

        
        virtual void STDMETHODCALLTYPE GetDefaultIoTarget( 
            /* [annotation][out] */ 
            _Out_  IWDFIoTarget **ppWdfIoTarget){
            printf("GetDefaultIoTarget\r\n");
        }

        
        virtual HRESULT STDMETHODCALLTYPE CreateWdfFile( 
            /* [annotation][string][unique][in] */ 
            _In_opt_  LPCWSTR pcwszFileName,
            /* [annotation][out] */ 
            _Out_  IWDFDriverCreatedFile **ppFile){
            printf("CreateWdfFile\r\n");
            return 0;
        }

        
        virtual void STDMETHODCALLTYPE GetDefaultIoQueue( 
            /* [annotation][out] */ 
            _Out_  IWDFIoQueue **ppWdfIoQueue){
            printf("GetDefaultIoQueue\r\n");
        }

        
        virtual HRESULT STDMETHODCALLTYPE CreateIoQueue( 
            /* [annotation][in] */ 
            _In_opt_  IUnknown *pCallbackInterface,
            /* [annotation][in] */ 
            _In_  BOOL bDefaultQueue,
            /* [annotation][in] */ 
            _In_  WDF_IO_QUEUE_DISPATCH_TYPE DispatchType,
            /* [annotation][in] */ 
            _In_  BOOL bPowerManaged,
            /* [annotation][in] */ 
            _In_  BOOL bAllowZeroLengthRequests,
            /* [annotation][out] */ 
            _Out_  IWDFIoQueue **ppIoQueue){
            printf("MyDevice::CreateIoQueue\r\n");
            *ppIoQueue = myQueue = new MyQueue(pCallbackInterface);
            printf("new queue=%p\r\n", *ppIoQueue);
            return 0;
        }

        
        virtual HRESULT STDMETHODCALLTYPE CreateDeviceInterface( 
            /* [annotation][in] */ 
            _In_  LPCGUID pDeviceInterfaceGuid,
            /* [annotation][unique][string][in] */ 
            _In_opt_  PCWSTR pReferenceString){
            //printf("CreateDeviceInterface\r\n");
            LPOLESTR str;
            StringFromIID(*pDeviceInterfaceGuid, &str);
            std::wcout << L"MyDevice::CreateDeviceInterface "
                        << str
                        << std::endl;
            return 0;
        }

        
        virtual HRESULT STDMETHODCALLTYPE AssignDeviceInterfaceState( 
            /* [annotation][in] */ 
            _In_  LPCGUID pDeviceInterfaceGuid,
            /* [annotation][unique][string][in] */ 
            _In_opt_  PCWSTR pReferenceString,
            /* [annotation][in] */ 
            _In_  BOOL Enable){
            //printf("AssignDeviceInterfaceState\r\n");
            LPOLESTR str;
            StringFromIID(*pDeviceInterfaceGuid, &str);
            std::wcout << L"MyDevice::AssignDeviceInterfaceState "
                        << str
                        << L"="
                        << Enable
                        << std::endl;
            return 0;
        }

        
        virtual HRESULT STDMETHODCALLTYPE RetrieveDeviceName( 
            /* [annotation][unique][out][string] */ 
            _Out_writes_to_opt_(*pdwDeviceNameLength, *pdwDeviceNameLength)  PWSTR pDeviceName,
            /* [annotation][out][in] */ 
            _Inout_  DWORD *pdwDeviceNameLength){
            printf("RetrieveDeviceName %p %lu\r\n", pDeviceName, *pdwDeviceNameLength);
            static const wchar_t name[] =  L"c:\\usb.txt";
            //static const wchar_t name[] =  L"c:\\UMDF.txt";
            if(pDeviceName) {
                wcscpy(pDeviceName, name);
                //Sleep(5000);
            }
            *pdwDeviceNameLength = sizeof(name);
            //Sleep(5000);
            return 0;
        }

        
        virtual HRESULT STDMETHODCALLTYPE PostEvent( 
            /* [annotation][in] */ 
            _In_  REFGUID EventGuid,
            /* [annotation][in] */ 
            _In_  WDF_EVENT_TYPE EventType,
            /* [annotation][size_is][in] */ 
            _In_reads_bytes_(cbDataSize)  BYTE *pbData,
            /* [annotation][in] */ 
            _In_  DWORD cbDataSize){
            printf("PostEvent\r\n");
            return 0;
        }

        
        virtual HRESULT STDMETHODCALLTYPE ConfigureRequestDispatching( 
            /* [annotation][in] */ 
            _In_  IWDFIoQueue *pQueue,
            /* [annotation][in] */ 
            _In_  WDF_REQUEST_TYPE RequestType,
            /* [annotation][in] */ 
            _In_  BOOL Forward){
            printf("MyDevice::ConfigureRequestDispatching\r\n");
            return 0;
        }

        
        virtual void STDMETHODCALLTYPE SetPnpState( 
            /* [annotation][in] */ 
            _In_  WDF_PNP_STATE State,
            /* [annotation][in] */ 
            _In_  WDF_TRI_STATE Value){
            printf("SetPnpState\r\n");
        }

        
        virtual WDF_TRI_STATE STDMETHODCALLTYPE GetPnpState( 
            /* [annotation][in] */ 
            _In_  WDF_PNP_STATE State){
            printf("GetPnpState\r\n");
            return WdfFalse;
        }

        
        virtual void STDMETHODCALLTYPE CommitPnpState( void){
            printf("CommitPnpState\r\n");
        }

        
        virtual HRESULT STDMETHODCALLTYPE CreateRequest( 
            /* [annotation][unique][in] */ 
            _In_opt_  IUnknown *pCallbackInterface,
            /* [annotation][unique][in] */ 
            _In_opt_  IWDFObject *pParentObject,
            /* [annotation][out] */ 
            _Out_  IWDFIoRequest **ppRequest){
            printf("CreateRequest\r\n");
            return 0;
        }

        
        virtual HRESULT STDMETHODCALLTYPE CreateSymbolicLink( 
            /* [annotation][unique][string][in] */ 
            _In_  PCWSTR pSymbolicLink){
            printf("CreateSymbolicLink\r\n");
            return 0;
        }

    // Device2
        virtual HRESULT STDMETHODCALLTYPE AssignS0IdleSettings( 
            /* [annotation][in] */ 
            _In_  WDF_POWER_POLICY_S0_IDLE_CAPABILITIES IdleCaps,
            /* [annotation][in] */ 
            _In_  DEVICE_POWER_STATE DxState,
            /* [annotation][in] */ 
            _In_  ULONG IdleTimeout,
            /* [annotation][in] */ 
            _In_  WDF_POWER_POLICY_S0_IDLE_USER_CONTROL UserControlOfIdleSettings,
            /* [annotation][in] */ 
            _In_  WDF_TRI_STATE Enabled){
            printf("AssignS0IdleSettings\r\n");
            return 0;
        }

        
        virtual HRESULT STDMETHODCALLTYPE StopIdle( 
            /* [annotation][in] */ 
            _In_  BOOL WaitForD0){
            printf("StopIdle\r\n");
            return 0;
        }

        
        virtual void STDMETHODCALLTYPE ResumeIdle( void){
            goIdle = 1;
            printf("ResumeIdle\r\n");
        }

        
        virtual HRESULT STDMETHODCALLTYPE CreateSymbolicLinkWithReferenceString( 
            /* [annotation][unique][string][in] */ 
            _In_  PCWSTR pSymbolicLink,
            /* [annotation][unique][string][in] */ 
            _In_opt_  PCWSTR pReferenceString){
            printf("CreateSymbolicLinkWithReferenceString\r\n");
            return 0;
        }

        
        virtual HRESULT STDMETHODCALLTYPE RegisterRemoteInterfaceNotification( 
            /* [annotation][in] */ 
            _In_  LPCGUID pDeviceInterfaceGuid,
            /* [annotation][in] */ 
            _In_  BOOL IncludeExistingInterfaces){
            printf("RegisterRemoteInterfaceNotification\r\n");
            return 0;
        }

        
        virtual HRESULT STDMETHODCALLTYPE CreateRemoteInterface( 
            /* [annotation][in] */ 
            _In_  IWDFRemoteInterfaceInitialize *pRemoteInterfaceInit,
            /* [annotation][unique][in] */ 
            _In_opt_  IUnknown *pCallbackInterface,
            /* [annotation][out] */ 
            _Out_  IWDFRemoteInterface **ppRemoteInterface){
            printf("CreateRemoteInterface\r\n");
            return 0;
        }

        
        virtual HRESULT STDMETHODCALLTYPE CreateRemoteTarget( 
            /* [annotation][unique][in] */ 
            _In_opt_  IUnknown *pCallbackInterface,
            /* [annotation][unique][in] */ 
            _In_opt_  IWDFObject *pParentObject,
            /* [annotation][out] */ 
            _Out_  IWDFRemoteTarget **ppRemoteTarget){
            printf("CreateRemoteTarget\r\n");
            return 0;
        }

        
        virtual void STDMETHODCALLTYPE GetDeviceStackIoTypePreference( 
            /* [annotation][out] */ 
            _Out_  WDF_DEVICE_IO_TYPE *ReadWritePreference,
            /* [annotation][out] */ 
            _Out_  WDF_DEVICE_IO_TYPE *IoControlPreference){
            printf("GetDeviceStackIoTypePreference\r\n");
        }

        
        virtual HRESULT STDMETHODCALLTYPE AssignSxWakeSettings( 
            /* [annotation][in] */ 
            _In_  DEVICE_POWER_STATE DxState,
            /* [annotation][in] */ 
            _In_  WDF_POWER_POLICY_SX_WAKE_USER_CONTROL UserControlOfWakeSettings,
            /* [annotation][in] */ 
            _In_  WDF_TRI_STATE Enabled){
            printf("AssignSxWakeSettings\r\n");
            return 0;
        }

        
        virtual POWER_ACTION STDMETHODCALLTYPE GetSystemPowerAction( void){
            printf("GetSystemPowerAction\r\n");
            return PowerActionNone;
        }

    // Device3
    public:
        virtual HRESULT STDMETHODCALLTYPE MapIoSpace( 
            /* [annotation][in] */ 
            _In_  PHYSICAL_ADDRESS PhysicalAddress,
            /* [annotation][in] */ 
            _In_  SIZE_T NumberOfBytes,
            /* [annotation][in] */ 
            _In_  MEMORY_CACHING_TYPE CacheType,
            /* [annotation][out] */ 
            _Out_  void **pPseudoBaseAddress){
            printf("MapIoSpace\r\n");
            return 0;
        }

        
        virtual void STDMETHODCALLTYPE UnmapIoSpace( 
            /* [annotation][in] */ 
            _In_  void *PseudoBaseAddress,
            /* [annotation][in] */ 
            _In_  SIZE_T NumberOfBytes){
            printf("UnmapIoSpace\r\n");
        }

        
        virtual void *STDMETHODCALLTYPE GetHardwareRegisterMappedAddress( 
            /* [annotation][in] */ 
            _In_  void *PseudoBaseAddress){
            printf("GetHardwareRegisterMappedAddress\r\n");
            return 0;
        }

        
        virtual SIZE_T STDMETHODCALLTYPE ReadFromHardware( 
            /* [annotation][in] */ 
            _In_  WDF_DEVICE_HWACCESS_TARGET_TYPE Type,
            /* [annotation][in] */ 
            _In_  WDF_DEVICE_HWACCESS_TARGET_SIZE Size,
            /* [annotation][in] */ 
            _In_  void *Address,
            /* [annotation][out] */ 
            _Out_writes_all_opt_(Count)  void *Buffer,
            /* [annotation][in] */ 
            _In_opt_  ULONG Count){
            printf("ReadFromHardware\r\n");
            return 0;
        }

        
        virtual void STDMETHODCALLTYPE WriteToHardware( 
            /* [annotation][in] */ 
            _In_  WDF_DEVICE_HWACCESS_TARGET_TYPE Type,
            /* [annotation][in] */ 
            _In_  WDF_DEVICE_HWACCESS_TARGET_SIZE Size,
            /* [annotation][in] */ 
            _In_  void *Address,
            /* [annotation][in] */ 
            _In_  SIZE_T Value,
            /* [annotation][in] */ 
            _In_reads_opt_(Count)  void *Buffer,
            /* [annotation][in] */ 
            _In_opt_  ULONG Count){
            printf("WriteToHardware\r\n");
        }

        
        virtual HRESULT STDMETHODCALLTYPE CreateInterrupt( 
            /* [annotation][in] */ 
            _In_  PWUDF_INTERRUPT_CONFIG Configuration,
            /* [annotation][out] */ 
            _Out_  IWDFInterrupt **ppInterrupt){
            printf("CreateInterrupt\r\n");
            return 0;
        }

        
        virtual HRESULT STDMETHODCALLTYPE CreateWorkItem( 
            /* [annotation][in] */ 
            _In_  PWUDF_WORKITEM_CONFIG pConfig,
            /* [annotation][in] */ 
            _In_  IWDFObject *pParentObject,
            /* [annotation][out] */ 
            _Out_  IWDFWorkItem **ppWorkItem){
            printf("CreateWorkItem\r\n");
            return 0;
        }

        
        virtual HRESULT STDMETHODCALLTYPE AssignS0IdleSettingsEx( 
            /* [annotation][in] */ 
            _In_  PWUDF_DEVICE_POWER_POLICY_IDLE_SETTINGS IdleSettings){
            printf("AssignS0IdleSettingsEx\r\n");
            return 0;
        }

        
};

struct MyDriver : public IWDFDriver {
    public:
        virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void **ppvObject) { 
            printf("QueryInterface\r\n");
            return 0; 
        }
        virtual ULONG STDMETHODCALLTYPE AddRef(void) { 
            printf("AddRef\r\n");
            return 0; 
        }
        virtual ULONG STDMETHODCALLTYPE Release(void) {
            printf("MyDriver::Release\r\n");
            return 0;
        }
    public:

        virtual HRESULT STDMETHODCALLTYPE DeleteWdfObject( void) {
            printf("DeleteWdfObject\r\n");
            return 0; 
        }
        
        virtual HRESULT STDMETHODCALLTYPE AssignContext( 
            /* [annotation][unique][in] */ 
            _In_opt_ __drv_aliasesMem  IObjectCleanup *pCleanupCallback,
            /* [annotation][unique][in] */ 
            _In_opt_ __drv_aliasesMem  void *pContext) { 
            printf("AssignContext\r\n");
            return 0;
        }
        
        virtual HRESULT STDMETHODCALLTYPE RetrieveContext( 
            /* [annotation][out] */ 
            _Out_  void **ppvContext) {
            printf("RetrieveContext\r\n");
            return 0;
        }
        
        virtual void STDMETHODCALLTYPE AcquireLock( void) {
            printf("AcquireLock\r\n");
        }
        
        virtual void STDMETHODCALLTYPE ReleaseLock( void) {
            printf("ReleaseLock\r\n");
        }

    public:
        virtual HRESULT STDMETHODCALLTYPE CreateDevice( 
            /* [annotation][in] */ 
            _In_  IWDFDeviceInitialize *pDeviceInit,
            /* [annotation][unique][in] */ 
            _In_opt_  IUnknown *pCallbackInterface,
            /* [annotation][out] */ 
            _Out_  IWDFDevice **ppDevice) {
            printf("CreateDevice\r\n");
            *ppDevice = myDevice = new MyDevice(pCallbackInterface);
            printf("new device=%p\r\n", *ppDevice);
            return 0;
        }
        
        virtual HRESULT STDMETHODCALLTYPE CreateWdfObject( 
            /* [annotation][unique][in] */ 
            _In_opt_  IUnknown *pCallbackInterface,
            /* [annotation][unique][in] */ 
            _In_opt_  IWDFObject *pParentObject,
            /* [annotation][out] */ 
            _Out_  IWDFObject **ppWdfObject) {
            printf("CreateWdfObject\r\n");
            return 0;
        }
        
        virtual HRESULT STDMETHODCALLTYPE CreatePreallocatedWdfMemory( 
            /* [annotation][size_is][in] */ 
            _In_reads_bytes_(BufferSize)  BYTE *pBuff,
            /* [annotation][in] */ 
            _In_  SIZE_T BufferSize,
            /* [annotation][unique][in] */ 
            _In_opt_  IUnknown *pCallbackInterface,
            /* [annotation][unique][in] */ 
            _In_opt_  IWDFObject *pParentObject,
            /* [annotation][out] */ 
            _Out_  IWDFMemory **ppWdfMemory) { 
            printf("CreatePreallocatedWdfMemory\r\n");
            return 0; 
        }
        
        virtual HRESULT STDMETHODCALLTYPE CreateWdfMemory( 
            /* [annotation][in] */ 
            _In_  SIZE_T BufferSize,
            /* [annotation][unique][in] */ 
            _In_opt_  IUnknown *pCallbackInterface,
            /* [annotation][unique][in] */ 
            _In_opt_  IWDFObject *pParentObject,
            /* [annotation][out] */ 
            _Out_  IWDFMemory **ppWdfMemory) { 
            printf("CreateWdfMemory\r\n");
            return 0;
        }
        
        virtual BOOL STDMETHODCALLTYPE IsVersionAvailable( 
            /* [annotation][in] */ 
            _In_  UMDF_VERSION_DATA *pMinimumVersion) { 
            printf("IsVersionAvailable\r\n");
            return true;
        }
        
        virtual HRESULT STDMETHODCALLTYPE RetrieveVersionString( 
            /* [annotation][unique][out][string] */ 
            _Out_writes_to_opt_(*pdwVersionLength, *pdwVersionLength)  PWSTR pVersion,
            /* [annotation][out][in] */ 
            _Inout_  DWORD *pdwVersionLength) {
            printf("RetrieveVersionString\r\n");
            return 0;
        }
};

class MyDevInit : public IWDFDeviceInitialize {
    public:
        virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void **ppvObject) { 
            printf("QueryInterface\r\n");
            return 0; 
        }
        virtual ULONG STDMETHODCALLTYPE AddRef(void) { 
            printf("AddRef\r\n");
            return 0; 
        }
        virtual ULONG STDMETHODCALLTYPE Release(void) {
            printf("Release\r\n");
            return 0;
        }

    public:
        virtual void STDMETHODCALLTYPE SetFilter( void) {
            printf("SetFilter\r\n");
        }
        
        virtual void STDMETHODCALLTYPE SetLockingConstraint( 
            /* [annotation][in] */ 
            _In_  WDF_CALLBACK_CONSTRAINT LockType) {
            printf("SetLockingConstraint\r\n");
        }
        
        virtual HRESULT STDMETHODCALLTYPE RetrieveDevicePropertyStore( 
            /* [annotation][unique][in] */ 
            _In_opt_  PCWSTR pcwszServiceName,
            /* [annotation][in] */ 
            _In_  WDF_PROPERTY_STORE_RETRIEVE_FLAGS Flags,
            /* [annotation][out] */ 
            _Out_  IWDFNamedPropertyStore **ppPropStore,
            /* [annotation][unique][out] */ 
            _Out_opt_  WDF_PROPERTY_STORE_DISPOSITION *pDisposition) {
            printf("RetrieveDevicePropertyStore\r\n");
            return 0;
        }
        
        virtual void STDMETHODCALLTYPE SetPowerPolicyOwnership( 
            /* [annotation][in] */ 
            _In_  BOOL fTrue) {
            printf("SetPowerPolicyOwnership\r\n");
        }
        
        virtual void STDMETHODCALLTYPE AutoForwardCreateCleanupClose( 
            /* [annotation][in] */ 
            _In_  WDF_TRI_STATE State) {
            printf("AutoForwardCreateCleanupClose\r\n");
        }
        
        virtual HRESULT STDMETHODCALLTYPE RetrieveDeviceInstanceId( 
            /* [annotation][unique][out][string] */ 
            _Out_opt_  PWSTR Buffer,
            /* [annotation][out][in] */ 
            _Inout_  DWORD *pdwSizeInChars) {
            printf("RetrieveDeviceInstanceId\r\n");
            return 0;
        }
        
        virtual void STDMETHODCALLTYPE SetPnpCapability( 
            /* [annotation][in] */ 
            _In_  WDF_PNP_CAPABILITY Capability,
            /* [annotation][in] */ 
            _In_  WDF_TRI_STATE Value) {
            printf("SetPnpCapability\r\n");
        }
        
        virtual WDF_TRI_STATE STDMETHODCALLTYPE GetPnpCapability( 
            /* [annotation][in] */ 
            _In_  WDF_PNP_CAPABILITY Capability) {
            printf("GetPnpCapability\r\n");
            return WdfFalse;
        }
};

std::basic_ostream<wchar_t> &
operator << (std::basic_ostream<wchar_t> &os, LARGE_INTEGER i)
{
    return os << i.QuadPart;
}


std::basic_ostream<wchar_t> &
operator << (std::basic_ostream<wchar_t> &os, WINBIO_REGISTERED_FORMAT r)
{
    return os << L"Owher=" << r.Owner << L", Type=" << r.Type;
}

void
identifyFeatureSet()
{
    //char ibuf[0x2000] = { 0 };
    unsigned char obuf[0x888];

    //MyMem in(ibuf, sizeof(ibuf)), out(obuf, sizeof(obuf));
    MyMem in(NULL, 0), out(obuf, sizeof(obuf));
    MyRequest req(WdfRequestOther, 0x442004, &out, &in);

    printf("about to 0x442004 - identifyFeatureSet\r\n");
    myQueue->ioctl->OnDeviceIoControl(myQueue, &req, 0x442004, 0, 0);
    while(!req.complete)
        Sleep(200);
    printf("Got back 0x%llx bytes: ", req.informationSize);
    for(LONG_PTR i=0;i<req.informationSize;i++)
        printf("%02x", obuf[i]);
    printf("\n");
}

void
identify()
{
    char ibuf[1024*10];
    WINBIO_CAPTURE_PARAMETERS *params = (WINBIO_CAPTURE_PARAMETERS *)ibuf;
    char obuf[1024*100];
    WINBIO_CAPTURE_DATA *data = (WINBIO_CAPTURE_DATA *)obuf;

    printf("sizeof(*params)=%lld\n", sizeof(*params));

    /*
0:002> d 41c49a6830
00000041`c49a6830  20 00 00 00 02 00 00 00-00 00 00 00 45 e2 a4 72   ...........E..r
00000041`c49a6840  20 fa dc 46 95 03 85 ba-d0 c0 27 a9 80 00 00 00   ..F......'.....
     */
    memset(ibuf, 0, sizeof(ibuf));
    params->PayloadSize = sizeof(*params);
    params->Purpose = WINBIO_PURPOSE_IDENTIFY;
    ((uint64_t*)&params->VendorFormat)[0] = 0x46DCFA2072A4E245L;
    ((uint64_t*)&params->VendorFormat)[1] = 0xA927C0D0BA850395L;
    params->Format.Owner = 0;//WINBIO_ANSI_381_FORMAT_OWNER;
    params->Format.Type = 0;//WINBIO_ANSI_381_FORMAT_TYPE;
    params->Flags = WINBIO_DATA_FLAG_PROCESSED;

    MyMem in(ibuf, sizeof(*params)), out(obuf, sizeof(obuf));
    MyRequest req(WdfRequestOther, IOCTL_BIOMETRIC_CAPTURE_DATA, &out, &in);

    printf("about to IOCTL_BIOMETRIC_CAPTURE_DATA\r\n");
    myQueue->ioctl->OnDeviceIoControl(myQueue, &req, IOCTL_BIOMETRIC_CAPTURE_DATA, 0, 0);
    //Sleep(5000);
    // printf("CANCEL!!!\n");
    //req.cancelCallback->OnCancel(&req);
    //printf("wakey wakey\r\n");
    //rc = myDevice->pnpcb->OnD0Entry(myDevice, WdfPowerDeviceInvalid);
    /*
0:007> d 86c8bbe4a0
00000086`c8bbe4a0  70 00 00 00 00 00 00 00-01 00 00 00 00 00 00 00  p...............
00000086`c8bbe4b0  58 00 00 00 30 00 00 00-20 00 00 00 00 00 00 00  X...0... .......
00000086`c8bbe4c0  00 00 00 00 00 00 00 00-00 00 00 00 00 00 00 00  ................
00000086`c8bbe4d0  00 00 00 00 00 14 00 00-00 00 00 00 00 00 00 00  ................
00000086`c8bbe4e0  00 02 fe 00 00 00 00 00-00 00 00 00 00 00 00 00  ................
00000086`c8bbe4f0  00 00 00 00 00 00 00 00-00 00 00 00 00 00 00 00  ................
00000086`c8bbe500  00 00 00 00 00 00 00 00-00 00 00 00 00 00 00 00  ................
    */
    while(!req.complete)
        Sleep(200);

    //Sleep(5000);
    
    std::wcout 
        << L"=======================" << std::endl
        << L"PayloadSize " << data->PayloadSize << std::endl
        << L"WinBioHresult " << data->WinBioHresult << std::endl
        << L"SensorStatus " << data->SensorStatus << std::endl
        << L"RejectDetail " << data->RejectDetail << std::endl
        << L"CaptureData.Size " << data->CaptureData.Size << std::endl
        << L"=======================" << std::endl
        ;
    if(data->CaptureData.Size > 0) {
        WINBIO_BIR *bir = (WINBIO_BIR *)data->CaptureData.Data;
        printf("bir->HeaderBlock = %ld, %ld\n", bir->HeaderBlock.Size, bir->HeaderBlock.Offset);
        printf("bir->StandardDataBlock = %ld, %ld\n", bir->StandardDataBlock.Size, bir->StandardDataBlock.Offset);
        printf("bir->VendorDataBlock = %ld, %ld\n", bir->VendorDataBlock.Size, bir->VendorDataBlock.Offset);
        printf("bir->SignatureBlock = %ld, %ld\n", bir->SignatureBlock.Size, bir->SignatureBlock.Offset);

        if(bir->HeaderBlock.Size > 0) {
            WINBIO_BIR_HEADER *hdr = (WINBIO_BIR_HEADER *)(data->CaptureData.Data+bir->HeaderBlock.Offset);

            std::wcout 
                << L"ValidFields = " << std::hex << hdr->ValidFields << std::endl
                << L"HeaderVersion = " << hdr->HeaderVersion << std::endl
                << L"PatronHeaderVersion = " << hdr->PatronHeaderVersion << std::endl
                << L"DataFlags = " << std::hex << hdr->DataFlags << std::endl
                << L"Type = " << hdr->Type << std::endl
                << L"Subtype = " << hdr->Subtype << std::endl
                << L"Purpose = " << hdr->Purpose << std::endl
                << L"DataQuality = " << (int)hdr->DataQuality << std::endl
                << L"CreationDate = " << hdr->CreationDate << std::endl
                << L"ValidityPeriod.BeginDate = " << hdr->ValidityPeriod.BeginDate << std::endl
                << L"ValidityPeriod.EndDate = " << hdr->ValidityPeriod.EndDate << std::endl
                << L"BiometricDataFormat = " << hdr->BiometricDataFormat << std::endl
                << L"ProductId = " << hdr->ProductId << std::endl
                ;
            printf("sz=%lld\n", (PUCHAR)(hdr+1) - data->CaptureData.Data);
        }
    }
    identifyFeatureSet();
}

void
commitEnrollment()
{
    //------------------------------- commit the enrollment --------------------------
    UCHAR arecord[] = {
        /* 4c, identity  */ 0x03, 0x00, 0x00, 0x00, 0x1c, 0x00, 0x00, 0x00, 0x01, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x05, 0x15, 0x00, 0x00, 0x00, 0xc5, 0x69, 0x85, 0x17, 0xbc, 0xff, 0x12, 0xe7, 0x24, 0x96, 0xb7, 0x63, 0xed, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        /* 04, subfactor */ 0xf6, 0x00, 0x00, 0x00,
        /* 08, payload sz*/ 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        /* 08, payload   */ 'U', 'n', 'i', 'c', 'o', 'r', 'n', 0x00
    };

    UCHAR obuf[1];
    MyMem in(arecord, sizeof(arecord)), out(obuf, sizeof(obuf));
    MyRequest req(WdfRequestOther, 0x442018, &out, &in);

    printf("about to Commit Enrollment\r\n");
    myQueue->ioctl->OnDeviceIoControl(myQueue, &req, 0x442018, 0, 0);
    while(!req.complete)
        Sleep(200);

    printf("Got back 0x%llx bytes: ", req.informationSize);
    for(LONG_PTR i=0;i<req.informationSize;i++)
        printf("%02x", obuf[i]);
    printf("\n");
}

void
reset()
{
    uint8_t buf[8];
    MyMem in(NULL, 0), out(buf, sizeof(buf));
    MyRequest req(WdfRequestOther, 0x440008, &out, &in);
    
    printf("about to Reset\r\n");
    myQueue->ioctl->OnDeviceIoControl(myQueue, &req, 0x440008, 0, 0);

    while(!req.complete)
        Sleep(200);

    Sleep(100);


    printf("Reset complete: ");
    for(int i=0;i<req.informationSize;i++)
        printf("%02x", buf[i]);
    printf("\r\n");
}

void
enroll()
{
    Sleep(500);
    
    //UCHAR ibuf[] = { 1, 0, 0, 0 };
    //MyMem in(ibuf, sizeof(ibuf)), out(NULL, 0);
    MyMem in(NULL, 0), out(NULL, 0);
    MyRequest req(WdfRequestOther, 0x44200C, &out, &in);

    printf("about to Create Enrollment\r\n");
    myQueue->ioctl->OnDeviceIoControl(myQueue, &req, 0x44200C, 0, 0);
    while(!req.complete)
        Sleep(200);

    Sleep(100);

    // keep going until template is complete
    for(int t=0;t<70;t++) {
        char ibuf[1024*10];
        WINBIO_CAPTURE_PARAMETERS *params = (WINBIO_CAPTURE_PARAMETERS *)ibuf;
        char obuf[1024*100];
        WINBIO_CAPTURE_DATA *data = (WINBIO_CAPTURE_DATA *)obuf;

        memset(ibuf, 0, sizeof(ibuf));
        params->PayloadSize = sizeof(*params);
        //params->Purpose = WINBIO_PURPOSE_IDENTIFY;
        params->Purpose = WINBIO_PURPOSE_ENROLL_FOR_IDENTIFICATION;
        ((uint64_t*)&params->VendorFormat)[0] = 0x46DCFA2072A4E245L;
        ((uint64_t*)&params->VendorFormat)[1] = 0xA927C0D0BA850395L;
        params->Format.Owner = 0;
        params->Format.Type = 0;
        params->Flags = WINBIO_DATA_FLAG_PROCESSED;

        MyMem in(ibuf, sizeof(*params)), out(obuf, sizeof(obuf));
        MyRequest req(WdfRequestOther, IOCTL_BIOMETRIC_CAPTURE_DATA, &out, &in);

        printf("about to IOCTL_BIOMETRIC_CAPTURE_DATA\r\n");
        myQueue->ioctl->OnDeviceIoControl(myQueue, &req, IOCTL_BIOMETRIC_CAPTURE_DATA, 0, 0);
        while(!req.complete)
            Sleep(200);

        std::wcout 
            << L"=======================" << std::endl
            << L"PayloadSize " << data->PayloadSize << std::endl
            << L"WinBioHresult " << data->WinBioHresult << std::endl
            << L"SensorStatus " << data->SensorStatus << std::endl
            << L"RejectDetail " << data->RejectDetail << std::endl
            << L"CaptureData.Size " << data->CaptureData.Size << std::endl
            << L"=======================" << std::endl
            ;

        // scan failed?
        if(data->SensorStatus == 2) {
            Sleep(100);
            continue;
        }

        Sleep(100);
        {
            UCHAR obuf[0x2c];
            MyMem in(NULL, 0), out(obuf, sizeof(obuf));
            MyRequest req(WdfRequestOther, 0x442010, &out, &in);

            printf("about to Update Enrollment\r\n");
            myQueue->ioctl->OnDeviceIoControl(myQueue, &req, 0x442010, 0, 0);
            while(!req.complete)
                Sleep(200);

            printf("Got back 0x%llx bytes: ", req.informationSize);
            for(LONG_PTR i=0;i<req.informationSize;i++)
                printf("%02x", obuf[i]);
            printf("\n");

            //printf("==============================================================================================\n");
            //printf("                           %d %% complete\n", obuf[36]);
            //printf("==============================================================================================\n");
            if(obuf[0] == 0)
                break;
        }
        Sleep(100);
    }

    //------------------------------- check for dups --------------------------
    {
        UCHAR obuf[0x50];
        MyMem in(NULL, 0), out(obuf, sizeof(obuf));
        MyRequest req(WdfRequestOther, 0x442014, &out, &in);

        printf("about to Check For Duplicate\r\n");
        myQueue->ioctl->OnDeviceIoControl(myQueue, &req, 0x442014, 0, 0);
        while(!req.complete)
            Sleep(200);

        printf("Got back 0x%llx bytes: ", req.informationSize);
        for(LONG_PTR i=0;i<req.informationSize;i++)
            printf("%02x", obuf[i]);
        printf("\n");
    }

    Sleep(1000);

    commitEnrollment();
}

void
getSensorStatus()
{
    char buf[1024*10];
    WINBIO_DIAGNOSTICS *diag = (WINBIO_DIAGNOSTICS*)buf;

    MyMem in(NULL, 0), out(buf, sizeof(buf));
    MyRequest req(WdfRequestOther, IOCTL_BIOMETRIC_GET_SENSOR_STATUS, &out, &in);

    printf("about to ioctl\r\n");
    myQueue->ioctl->OnDeviceIoControl(myQueue, &req, IOCTL_BIOMETRIC_GET_SENSOR_STATUS, 0, 0);
    while(!req.complete)
        Sleep(200);
    //Sleep(4000);
    //rc = myDevice->pnpcb->OnD0Entry(myDevice, WdfPowerDeviceInvalid);
    //Sleep(4000);
    std::wcout 
        << L"=======================" << std::endl
        << L"PayloadSize " << diag->PayloadSize << std::endl
        << L"WinBioHresult " << diag->WinBioHresult << std::endl
        << L"SensorStatus " << diag->SensorStatus << std::endl
        << L"VendorDiagnostics.Size " << diag->VendorDiagnostics.Size << std::endl
        << L"=======================" << std::endl
        ;
    
}

void
getAttributes()
{
    char obuf[10*1024];
    WINBIO_SENSOR_ATTRIBUTES *attrs = (WINBIO_SENSOR_ATTRIBUTES*)obuf;
    MyMem in(NULL, 0), out(obuf, sizeof(obuf));
    MyRequest req(WdfRequestOther, IOCTL_BIOMETRIC_GET_ATTRIBUTES, &out, &in);

    printf("about to ioctl\r\n");
    myQueue->ioctl->OnDeviceIoControl(myQueue, &req, IOCTL_BIOMETRIC_GET_ATTRIBUTES, 0, 0);
    //Sleep(1000);
    //rc = myDevice->pnpcb->OnD0Entry(myDevice, WdfPowerDeviceInvalid);
    while(!req.complete)
        Sleep(200);
    printf("WinBioHresult = %lx\r\n", attrs->WinBioHresult);
    std::wcout 
        << L"PayloadSize: " << attrs->PayloadSize << std::endl
        << L"ManufacturerName: " << attrs->ManufacturerName  << std::endl
        << L"ModelName: " << attrs->ModelName  << std::endl
        << L"SensorType: " << attrs->SensorType << std::endl
        << L"SensorSubType: " << attrs->SensorSubType << std::endl
        << L"Capabilities: " << attrs->Capabilities << std::endl
        << L"SerialNumber: " << attrs->SerialNumber << std::endl
        << L"FirmwareVersion: " << attrs->FirmwareVersion.MajorVersion << "." << attrs->FirmwareVersion.MinorVersion << std::endl
        << L"SupportedFormatEntries: " << attrs->SupportedFormatEntries << std::endl
        << std::endl;
        for(unsigned int i=0;i<attrs->SupportedFormatEntries;i++) {
            printf("  Owner=%04x, Type=%04x\n", 
                    attrs->SupportedFormat[i].Owner, 
                    attrs->SupportedFormat[i].Type);
        }
}

void
setMode(unsigned short mode)
{
    uint32_t ibuf[2] = { mode, 2 };

    MyMem in((unsigned char*)ibuf, sizeof(ibuf)), out(NULL, 0);
    MyRequest req(WdfRequestOther, 0x44204C, &out, &in);

    printf("about to setMode\r\n");
    myQueue->ioctl->OnDeviceIoControl(myQueue, &req, 0x44204C, 0, 0);
    while(!req.complete)
        Sleep(200);
    printf("setMode - complete\n");
}

void
deleteRecord()
{
        // delete record
    unsigned char ibuf[0x50] = { 
        /* 4c, identity  */ 0x03, 0x00, 0x00, 0x00, 0x1c, 0x00, 0x00, 0x00, 0x01, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x05, 0x15, 0x00, 0x00, 0x00, 0xc5, 0x69, 0x85, 0x17, 0xbc, 0xff, 0x12, 0xe7, 0x24, 0x96, 0xb7, 0x63, 0xed, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        /* 04, subfactor */ 0xf6, 0x00, 0x00, 0x00,
    };
    unsigned char obuf[0x08] = { 0 };

    MyMem in(ibuf, sizeof(ibuf)), out(obuf, sizeof(obuf));
    MyRequest req(WdfRequestOther, 0x442034, &out, &in);

    printf("about to 442034\r\n");
    myQueue->ioctl->OnDeviceIoControl(myQueue, &req, 0x442034, 0, 0);
    while(!req.complete)
        Sleep(200);
    printf("Got back 0x%llx bytes: ", req.informationSize);
    for(LONG_PTR i=0;i<req.informationSize;i++)
        printf("%02x", obuf[i]);
    printf("\n");
}

void
discardEnrollment()
{
    MyMem in(NULL, 0), out(NULL, 0);
    MyRequest req(WdfRequestOther, 0x44201C, &out, &in);

    printf("about to Discard Enrollment\r\n");
    myQueue->ioctl->OnDeviceIoControl(myQueue, &req, 0x44201C, 0, 0);
    while(!req.complete)
        Sleep(200);
}

void
setIndicator()
{
    HRESULT rc;
    WINBIO_SET_INDICATOR setInd;
    setInd.PayloadSize = sizeof(setInd);
    setInd.IndicatorStatus = WINBIO_INDICATOR_ON;
    WINBIO_GET_INDICATOR getInd;
    MyMem in(&setInd, sizeof(setInd)), out(&getInd, sizeof(getInd));
    MyRequest req(WdfRequestOther, IOCTL_BIOMETRIC_SET_INDICATOR, &out, &in);

    std::wcout
        << L"==== Before ===" << std::endl
        << L"setInd.PayloadSize: " << setInd.PayloadSize << std::endl
        << L"setInd.IndicatorStatus: " << setInd.IndicatorStatus << std::endl
        ;
    printf("about to ioctl\r\n");
    myQueue->ioctl->OnDeviceIoControl(myQueue, &req, IOCTL_BIOMETRIC_SET_INDICATOR, sizeof(setInd), sizeof(setInd));
    Sleep(1000);
    rc = myDevice->pnpcb->OnD0Entry(myDevice, WdfPowerDeviceInvalid);

    std::wcout 
        << L"rc=" << rc << std::endl
        << L"==== After ===" << std::endl
        << L"getInd.PayloadSize: " << getInd.PayloadSize << std::endl
        << L"getInd.IndicatorStatus: " << getInd.IndicatorStatus << std::endl
        ;
}

void
getDatabaseSize()
{
    unsigned char buf[8] = {0};
    MyMem in(NULL, 0), out(buf, sizeof(buf));
    MyRequest req(WdfRequestOther, 0x44202C, &out, &in);

    printf("about to ioctl StorageAdapterGetDatabaseSize\r\n");
    myQueue->ioctl->OnDeviceIoControl(myQueue, &req, 0x44202C, 0, sizeof(buf));
    while(!req.complete)
        Sleep(200);
    /*
    Sleep(4000);
    rc = myDevice->pnpcb->OnD0Entry(myDevice, WdfPowerDeviceInvalid);
    Sleep(4000);
    */
    for(unsigned int i=0;i<sizeof(buf);i++) {
        printf("%02x", buf[i]);
    }
    printf("\n");
}

void
blah()
{
    puts("blah");
}

void
handle_blah(_EXCEPTION_POINTERS *ExceptionInfo)
{
    printf("Yay! Breakpoints work\n");
}

void
handle_trace(_EXCEPTION_POINTERS *ExceptionInfo)
{
    PCONTEXT ctx = ExceptionInfo->ContextRecord;
    
    printf("Trace: %s:%lld\n", ctx->Rcx, ctx->Rdx);
}

void
handle_reset_calib_data_and_calibrate(_EXCEPTION_POINTERS *ExceptionInfo)
{
    printf("reset_calib_data_and_calibrate:\n");
    print_regs(ExceptionInfo);
}

void
handle_calibrate_iteration(_EXCEPTION_POINTERS *ExceptionInfo)
{
    printf("==========================================================================================\n");
    printf("                          PHASE %d\n", ExceptionInfo->ContextRecord->Rdx);
    printf("==========================================================================================\n");
}

const uint8_t target[] = { 
//0x4e, 0x00, 0x28, 0x00, 0xfb, 0xb2, 0x0f, 0x00, 0x00, 0x00, 0x0f, 0x00, 0x30, 0x00, 0x00, 0x00, 0x87, 0x00, 0x02, 0x00, 0x67, 0x00, 0x0a, 0x00, 0x01, 0x80, 0x00, 0x00, 0x0a, 0x02, 0x00, 0x00, 0x0b, 0x19, 0x00, 0x00, 0x88, 0x13, 0xb8, 0x0b, 0x01, 0x09, 0x10, 0x00, 
//0x17, 0x00, 0x00, 0x00, 
'n', 0x23, 0x00, 0x00, 0x00, 0x20, 0x00, 0x08, 0x00, 0x00, 0x20, 0x00, 0x80, 0x00, 0x00, 0x01, 0x00, 0x32, 0x00, 0x74, 0x00, 0x00, 0x00, 0x00, 0x80, 0x20, 0x20, 0x04, 0x00, 0x24, 0x20, 0x00, 0x00, 0x50, 0x20, 0x77, 0x36, 0x28, 0x20, 0x01, 0x00, 0x30, 0x20, 0x01, 0x00, 0x3c, 0x20, 0x80, 0x00, 0x08, 0x21, 0x38, 0x00, 0x0c, 0x21, 0x00, 0x00, 0x48, 0x21, 0x07, 0x00, 0x4c, 0x21, 0x00, 0x00, 0x58, 0x20, 0x00, 0x00, 0x5c, 0x20, 0x00, 0x00, 0x60, 0x20, 
};


void
handle_malloc(_EXCEPTION_POINTERS *ExceptionInfo)
{
    PCONTEXT ctx = ExceptionInfo->ContextRecord;
    uint64_t *rsp = (uint64_t *)ctx->Rsp;
    uint8_t *rax = (uint8_t *)ctx->Rax;
    uint32_t len = (uint32_t)rsp[5+1];
    size_t i;
    
    printf("malloc %ld: %p\n", len, rax);
    if(len == 13440) {

        for(i=0;i<40;i++) {
            printf("    %016llx\n", rsp[i]);
        }
    }

    if(len == 728) {
        printf("Allocating BiometricDevice!\n");
    }
}

void
handle_memmove(_EXCEPTION_POINTERS *ExceptionInfo)
{
    PCONTEXT ctx = ExceptionInfo->ContextRecord;
    uint64_t *rsp = (uint64_t *)ctx->Rsp;
    uint8_t *dst = (uint8_t*)rsp[1+5];
    uint8_t *src = (uint8_t*)rsp[2+5];
    size_t len = (uint32_t)rsp[3+5];
    //uint8_t *src = (uint8_t*)ctx->Rdx;
    //uint8_t *dst = (uint8_t*)ctx->Rcx;
    //size_t len = ctx->R8;
    size_t i;

    printf("memmove %p -> %p (%lld): ", src, dst, len);
    for(i=0;i<len;i++)
        printf("%02x", src[i]);
    puts("");

    if(len >= sizeof(target) && memcmp(src, target, sizeof(target)) == 0) {
        for(i=0;i<40;i++) {
            printf("    %016llx\n", rsp[i]);
        }
    }
}

void
handle_x(_EXCEPTION_POINTERS *ExceptionInfo)
{
    PCONTEXT ctx = ExceptionInfo->ContextRecord;
    uint64_t *rsp = (uint64_t *)ctx->Rsp;
    uint8_t *src = (uint8_t*)ctx->Rcx;
    int i;

    printf("set sensor type before: ");
    for(i=0;i<0x40;i++)
        printf("%02x", src[i]);
    puts("");
}

void
handle_x_end(_EXCEPTION_POINTERS *ExceptionInfo)
{
    PCONTEXT ctx = ExceptionInfo->ContextRecord;
    uint64_t *rsp = (uint64_t *)ctx->Rsp;
    uint8_t *src, *dst;
    int i;

    //print_regs(ExceptionInfo);
    rsp += 0x78/8;
    printf("arg_0=%016llx\n", rsp[0x8/8]);
    printf("arg_8=%016llx\n", rsp[0x10/8]);
    printf("arg_10=%016llx\n", rsp[0x18/8]);
    printf("arg_18=%016llxd\n", rsp[0x20/8]);

    src = (uint8_t *)rsp[0x8/8];
    printf("set sensor type after: ");
    for(i=0;i<0x40;i++)
        printf("%02x", src[i]);
    puts("");
}

void
print_biometric_device(uint64_t *rcx)
{
    size_t i, j;

    printf("Biometric device: %p\n", rcx);
    for(i=0;i<140;i++) {
        printf("    %04llx: 0x%016llx\n", i*8, rcx[i]);
        if(i*8 == 0x28) { // device/fw info (as returned by cmd_01)
            uint64_t *f28 = (uint64_t *)rcx[i];
            for(j=0;j<10;j++) {
                printf("        %04llx: 0x%016llx\n", j*8, f28[j]);
            }
        }

        if(i*8 == 0x288) { // calibration info
            uint64_t *f28 = (uint64_t *)rcx[i];
            for(j=0;j<40;j++) {
                printf("        %04llx: 0x%016llx\n", j*8, f28[j]);
                if(j*8 == 0x80 && f28[j]) {
                    uint8_t *cal_blob = (uint8_t *)f28[j];
                    int k;
                    printf("            Calibration blob: ");
                    for(k=0;k<0x80;k++) {
                        printf("%02x", cal_blob[k]);
                    }
                    printf("...\n");
                }

                if(j*8 == 0x68 && f28[j]) {
                    uint8_t *cal_blob = (uint8_t *)f28[j];
                    int k;
                    printf("            field_68: ");
                    for(k=0;k<0x80;k++) {
                        printf("%02x", cal_blob[k]);
                    }
                    printf("...\n");
                }

                if(j*8 == 0x98 && f28[j]) {
                    uint8_t *cal_blob = (uint8_t *)f28[j];
                    int k;
                    printf("            Calibration blob2: ");
                    for(k=0;k<0x80;k++) {
                        printf("%02x", cal_blob[k]);
                    }
                    printf("...\n");
                }
            }
        }
    }
}

void
handle_line_update(_EXCEPTION_POINTERS *ExceptionInfo)
{
    PCONTEXT ctx = ExceptionInfo->ContextRecord;
    uint64_t *rsp = (uint64_t *)ctx->Rsp;
    uint64_t *rcx = (uint64_t *)ctx->Rcx;

    //print_regs(ExceptionInfo);
    print_biometric_device(rcx);
}

void
pp(uint64_t *tmp)
{
    printf(" %016llx %016llx\n", tmp[0], tmp[1]);
}

void
handle_create_line_transform_segment(_EXCEPTION_POINTERS *ExceptionInfo)
{
    PCONTEXT ctx = ExceptionInfo->ContextRecord;
    uint64_t *rsp = (uint64_t *)ctx->Rsp;
    size_t i, j;

    pp((uint64_t *)ctx->Rcx);
    pp((uint64_t *)ctx->Rdx);
    pp((uint64_t *)ctx->R8);
    pp((uint64_t *)ctx->R9);
    pp((uint64_t *)rsp[6]);
    pp((uint64_t *)rsp[7]);

    print_regs(ExceptionInfo);
    for(i=0;i<40;i++) {
        printf("    %016llx\n", rsp[i]);
    }
}

void
handle_z(_EXCEPTION_POINTERS *ExceptionInfo)
{
    PCONTEXT ctx = ExceptionInfo->ContextRecord;
    uint8_t *src = (uint8_t*)ctx->Rax;
    int i;

    print_regs(ExceptionInfo);
    printf("z: ");
    for(i=0;i<0x20;i++)
        printf("%02x", src[i]);
    puts("");
}

void
handle_avg(_EXCEPTION_POINTERS *ExceptionInfo)
{
    PCONTEXT ctx = ExceptionInfo->ContextRecord;
    // 58h
    uint64_t *rsp = (uint64_t *)ctx->Rsp;
    uint8_t *src = (uint8_t *)rsp[(0x58+0x10)/8];
    uint64_t *dst_struct = (uint64_t *)rsp[(0x58+0x30)/8];
    uint8_t *dst = (uint8_t *)dst_struct[0x10/8];
    int i;

    print_regs(ExceptionInfo);
    printf("In: ");
    for(i=0;i<0x200;i++)
        printf("%02x", src[i]);
    puts("");
    printf("Out: ");
    for(i=0;i<0x200;i++)
        printf("%02x", dst[i]);
    puts("");
}

void
handle_scale_calibration_buffer_start(_EXCEPTION_POINTERS *ExceptionInfo)
{
    PCONTEXT ctx = ExceptionInfo->ContextRecord;
    uint64_t *biometricDevice = *(uint64_t **)ctx->Rcx;
    uint64_t *rsp = (uint64_t *)ctx->Rsp;
    uint64_t *calib_results;
    uint8_t *src;
    int i;

    calib_results= biometricDevice + 0xd8/8;
    src = (uint8_t *)calib_results[0x10/8];
    if(src) {
        printf("In: ");
        for(i=0;i<0x200;i++)
            printf("%02x", src[i]);
        puts("");
    }
}

void
handle_scale_calibration_buffer_end(_EXCEPTION_POINTERS *ExceptionInfo)
{
    PCONTEXT ctx = ExceptionInfo->ContextRecord;
    uint64_t *biometricDevice;
    uint64_t *rsp = (uint64_t *)ctx->Rsp;
    uint64_t *calib_results, *calib_info;
    uint8_t *src, *dst;
    int i;

    rsp += 0xc8/8;
    printf("arg_28=%d\n", (uint16_t)rsp[0x30/8]);
    printf("arg_8=%d\n", (uint16_t)rsp[0x10/8]);
    printf("arg_10=%d\n", (uint16_t)rsp[0x18/8]);

    biometricDevice = (uint64_t *)rsp[8/8];
    if(biometricDevice) {
        biometricDevice = (uint64_t *)*biometricDevice;
        calib_results= biometricDevice + 0xd8/8;
        src = (uint8_t *)calib_results[0x10/8];
        if(src) {
            printf("In: ");
            for(i=0;i<0x200;i++)
                printf("%02x", src[i]);
            puts("");
        }

        calib_info = (uint64_t *)biometricDevice[0x288/8];
        dst = (uint8_t *)calib_info[0x80/8];
        printf("Out: ");
        for(i=0;i<0x200;i++)
            printf("%02x", dst[i]);
        puts("");
    }
}

void
handle_sub_180067360(_EXCEPTION_POINTERS *ExceptionInfo)
{
    PCONTEXT ctx = ExceptionInfo->ContextRecord;
    uint64_t *biometricDevice;
    uint64_t *rsp = (uint64_t *)ctx->Rsp;
    uint64_t *calib_results, *calib_info;
    uint8_t *src, *dst;
    int i;

    print_regs(ExceptionInfo);
    rsp += 0x58/8;
    printf("arg_0=%016llx\n", rsp[0x8/8]);
    printf("arg_8=%016llx\n", rsp[0x10/8]);
    printf("arg_10=%016llx\n", rsp[0x18/8]);
    printf("arg_18=%016llxd\n", rsp[0x20/8]);

    biometricDevice = (uint64_t *)rsp[8/8];
    if(biometricDevice) {
        biometricDevice = (uint64_t *)*biometricDevice;
        if(!biometricDevice)
            puts("biometricDevice is null");
        calib_results= biometricDevice + 0xd8/8;
        src = (uint8_t *)calib_results[0x10/8];
        if(src) {
            printf("Avg: ");
            for(i=0;i<0x200;i++)
                printf("%02x", src[i]);
            puts("");
        }

        calib_info = (uint64_t *)biometricDevice[0x288/8];
        if(calib_info) {
            dst = (uint8_t *)calib_info[0x80/8];
            if(dst) {
                printf("Line Update Info > calibration blob: ");
                for(i=0;i<0x200;i++)
                    printf("%02x", dst[i]);
                puts("");
            }
        }
    }
}

void
handle_hack_timeslot_table_for_regwrite_8000203c(_EXCEPTION_POINTERS *ExceptionInfo)
{
    PCONTEXT ctx = ExceptionInfo->ContextRecord;
    uint64_t **rcx = (uint64_t **)ctx->Rcx;

    print_biometric_device(*rcx);
}

void
print_axbuf(_EXCEPTION_POINTERS *ExceptionInfo)
{
    PCONTEXT ctx = ExceptionInfo->ContextRecord;
    uint8_t *rax= (uint8_t *)ctx->Rax;
    int i;

    printf("rax: ");
    for(i=0;i<0x20;i++)
        printf("%02x", rax[i]);
    puts("");
}

void
print_cxbuf(_EXCEPTION_POINTERS *ExceptionInfo)
{
    PCONTEXT ctx = ExceptionInfo->ContextRecord;
    uint8_t *rcx= (uint8_t *)ctx->Rcx;
    int i;

    print_regs(ExceptionInfo);
    printf("rcx: ");
    for(i=0;i<0x400;i++)
        printf("%02x", rcx[i]);
    puts("");
}


uint8_t *tout;
uint32_t *tsize;

void
handle_export_in(_EXCEPTION_POINTERS *ExceptionInfo)
{
    PCONTEXT ctx = ExceptionInfo->ContextRecord;
    tout = (uint8_t *)ctx->R8;
    tsize = (uint32_t *)ctx->R9;

    print_regs(ExceptionInfo);
}


void
handle_export_out(_EXCEPTION_POINTERS *ExceptionInfo)
{
    PCONTEXT ctx = ExceptionInfo->ContextRecord;
    int i;

    if(!tout || !tsize)
        return;

    print_regs(ExceptionInfo);
    printf("tout: ");
    for(i=0;i<*tsize;i++)
        printf("%02x", tout[i]);
    puts("");
}


void
handle_new_biodev(_EXCEPTION_POINTERS *ExceptionInfo)
{
    PCONTEXT ctx = ExceptionInfo->ContextRecord;

    print_regs(ExceptionInfo);
    ctx->Dr0 = ctx->Rax+0x128; // bytes per line
    ctx->Dr7 = (1 << 0) | (1 << 16) | (3 << 18);
    printf("hw breakpoint set to %016llx\n", ctx->Dr0);
}

void
handle_lookup_capture_blob(_EXCEPTION_POINTERS *ExceptionInfo)
{
    PCONTEXT ctx = ExceptionInfo->ContextRecord;
    uint64_t *rsp = (uint64_t *)ctx->Rsp;
    int i;

    print_regs(ExceptionInfo);
    rsp += 0x98/8;
    for(i=0;i<10;i++) {
        printf("arg_%x=%016llx\n", i*8, rsp[i+1]);
    }
}


struct breakpoint breakpoints_0097[] = {
    { "blah", (unsigned char *)blah, handle_blah },
    { NULL, (unsigned char *)0x1800409B0, handle_trace },
    { "handle_reset_calib_data_and_calibrate", (unsigned char *)0x18005FD40, handle_reset_calib_data_and_calibrate },
    { "handle_calibrate_iteration", (unsigned char *)0x000000018006DF20, handle_calibrate_iteration },
    { NULL, (unsigned char *)0x000000018003E6F9, handle_memmove },
    { "handle_line_update", (unsigned char *)0x000000018008F950, handle_line_update},
    { "print_regs", (unsigned char *)0x000000018009D2D4, print_regs },
    { "z", (unsigned char *)0x000000018008E326, handle_z },
    { NULL,(unsigned char *)0x000000018003E623, handle_malloc },
    { "handle_avg", (unsigned char *)0x0000000180093FC8, handle_avg },
    { "handle_scale_calibration_buffer_start", (unsigned char *)0x0000000180071AC0, handle_scale_calibration_buffer_start }, // scale_calibration_buffer
    { "handle_scale_calibration_buffer_end", (unsigned char *)0x00000001800723F9, handle_scale_calibration_buffer_end }, // scale_calibration_buffer
    { "scale_calibration_buffer", (unsigned char *)0x00000001800746B0, print_regs }, // subfunction of scale_calibration_buffer
    { "handle_sub_180067360_start", (unsigned char *)0x0000000180067377, handle_sub_180067360 }, // called from scale_calibration_buffer 3 times
    { "handle_sub_180067360_end", (unsigned char *)0x0000000180067597, handle_sub_180067360 }, // called from scale_calibration_buffer 3 times
    { "hack_timeslot_table_for_regwrite_8000203c", (unsigned char *)0x0000000180087540, handle_hack_timeslot_table_for_regwrite_8000203c},
    { "maybe set sensor type", (unsigned char *)0x0000000180066AAE, print_regs },
    { "00000001800666B0 start", (unsigned char *)0x00000001800666B0, handle_x }, // sets the sensor type from arg0+0x10
    { "00000001800666B0 end", (unsigned char *)0x0000000180066AC2, handle_x_end }, // sets the sensor type from arg0+0x10
    { "bytes per line", (unsigned char *)0x000000018007F94F, print_regs },
    { "bytes per line 2", (unsigned char *)0x000000018007F93A, print_axbuf },
    { "new BiometricDevice, after memset", (unsigned char *)0x000000018007EE83, handle_new_biodev },
    { "lookup capture blob", (unsigned char *)0x0000000180094B8A, handle_lookup_capture_blob },
    //{ "avg, no overscan", (unsigned char *)0x0000000180093EB7, handle_avg_overscan },
    { 0 }
};

struct breakpoint breakpoints[] = {
    { "CeivMode::IdentifyUserStorageOnFlash", (unsigned char *)0x0000000180017531, print_regs },
    /*
    { "create_MVWT", (unsigned char *)0x00000001800D7886, print_cxbuf },
    { "00000001800E1330", (unsigned char *)0x00000001800E1330, print_cxbuf },
    { "00000001800F5B73", (unsigned char *)0x00000001800F5B73, print_regs },
    { "00000001800FA054", (unsigned char *)0x00000001800FA054, print_regs },
    { "00000001800F71A2", (unsigned char *)0x00000001800F71A2, print_regs },
    */
    { "00000001800D8C24", (unsigned char *)0x00000001800D8C24, handle_export_in },
    { "00000001800D8C34", (unsigned char *)0x00000001800D8C34, handle_export_out },
};

void
nop()
{
}

void
usage()
{
    puts("Usage: wine a.exe <nop|identify|enroll>");
}


int
main(int argc, char *argv[])
{
    char *br = getenv("SANDBOX_BREAKPOINTS");
    void (*what)();
    IClassFactory *fact = 0;

#if 0
    if(argc != 2) {
        usage();
        return 3;
    }

    if(strcasecmp(argv[1], "identify") == 0) {
        what = identify;
    }
    else if(strcasecmp(argv[1], "enroll") == 0) {
        what = enroll;
    }
    else if(strcasecmp(argv[1], "nop") == 0) {
        what = nop;
    }
    else {
        usage();
        return 3;
    }
#endif

    HMODULE pDll = LoadLibrary("synawudfbiousb.dll");
    if(!pDll) {
        puts("Failed to LoadLibrary dll");
        return 3;
    }
    DllGetClassObject_t *proc = (DllGetClassObject_t*)GetProcAddress(pDll, "DllGetClassObject");
    if(!proc) {
        puts("DllGetClassObject was not exported from the synawudfbiousb.dll");
        return 3;
    }
    printf("about to create factory\r\n");
    proc(SYNA_CLSID, IID_IUnknown, (LPVOID *)&fact);
    LPVOID dibr = 0;
    proc(GUID_DEVINTERFACE_BIOMETRIC_READER, IID_IUnknown, (LPVOID *)&dibr);
    printf("GUID_DEVINTERFACE_BIOMETRIC_READER=%p\n", dibr);
    //Sleep(5000);
    //DllGetClassObject(SYNA_CLSID, IID_IUnknown, (LPVOID *)&fact);
    IDriverEntry *inst;
    printf("about to create instance fact = %p\r\n", fact);
    if(!fact) {
        puts("SYNA_CLSID not found in DLL");
        return 3;
    }
    fact->CreateInstance(NULL, IID_IDriverEntry, (LPVOID *)&inst);

/*
    unsigned char *trace_flags = (unsigned char *)0x0000000180233E20;
    trace_flags[1]=0x7f;
    trace_flags[4]=0x7f;
    printf("*0000000180233E20: %p\n", *(PVOID*)0x0000000180233E20);
    //return 1;
    unsigned char *trace_flags = (unsigned char *)0x000000180240820;
    trace_flags[1]=0x7f;
    trace_flags[4]=0x7f;
    printf("*0000000180240820: %p\n", *(PVOID*)0x0000000180240820);
*/

    HRESULT rc;

    MyDriver *aDriver = new MyDriver();
    printf("about to init %p\r\n", aDriver);
    rc = inst->OnInitialize(aDriver);
    printf("OnInitialize rc = %lx\r\n", rc);
    if(rc < 0) {
        return 0;
    }
    Sleep(100);

    // Driver loaded, instument the code
    if(br && strcmp(br, "1") == 0) {
        set_bps(breakpoints);

        // Test breakpoints
        blah();
        blah();
    }

    MyDevInit *devinit = new MyDevInit();
    printf("about to add device %p\r\n", devinit);
    rc = inst->OnDeviceAdd(aDriver, devinit);
    printf("OnDeviceAdd rc = %lx\r\n", rc);
    if(rc < 0) {
        return 0;
    }

    goIdle = 0;

    Sleep(100);
    printf("about to prepare hw\r\n");
    rc = myDevice->pnphwcb->OnPrepareHardware(myDevice);
    printf("OnPrepareHardware rc = %lx\r\n", rc);

    if(rc < 0) {
        return 0;
    }

    Sleep(100);
    printf("about to enter D0 state\r\n");
    rc = myDevice->pnpcb->OnD0Entry(myDevice, WdfPowerDeviceInvalid);
    printf("OnD0Entry rc = %lx\r\n", rc);

    if(rc < 0) {
        return 0;
    }

#if 0
    printf("about to release hw\r\n");
    rc = myDevice->pnphwcb->OnReleaseHardware(myDevice);
    printf("OnReleaseHardware rc = %lx\r\n", rc);

    if(rc < 0) {
        return 0;
    }
#endif

    puts("All done, sleeping");
    while(!goIdle) {
        Sleep(200);
    }

//    reset();

    setMode(2); // WINBIO_SENSOR_ADVANCED_MODE

    //what();
    enroll();
    
    printf("======================================================\r\n");
    printf("Sleeping 10 secons....\r\n");
    printf("======================================================\r\n");
    Sleep(10000);

    identify();

    printf("about to de-init\r\n");
    inst->OnDeinitialize(aDriver);
}
