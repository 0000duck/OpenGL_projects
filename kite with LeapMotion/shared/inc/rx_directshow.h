/*! 
  @file rx_directshow.h
	
  @brief DirectShow���g�����L���v�`��
   ���Ёu�͂��߂Ă̓��揈���v���O���~���O�v�y�� ���M �� CQ�o��
   http://www.cqpub.co.jp/hanbai/books/43/43001.htm
 
  @author Eiichiro Momma,Kenji Takahashi,Makoto Fujisawa
  @date 2007-07-04,2007-11-16,2011-12-12
*/

//-----------------------------------------------------------------------------
// �C���N���[�h�t�@�C��
//-----------------------------------------------------------------------------
#include <dshow.h>
#pragma include_alias( "dxtrans.h", "qedit.h" )
#define __IDxtCompositor_INTERFACE_DEFINED__
#define __IDxtAlphaSetter_INTERFACE_DEFINED__
#define __IDxtJpeg_INTERFACE_DEFINED__
#define __IDxtKey_INTERFACE_DEFINED__
#include <qedit.h>
#pragma comment(lib,"strmiids.lib")

#include <vector>
#include <string>

using namespace std;


//-----------------------------------------------------------------------------
// rxDirectShow�N���X
//-----------------------------------------------------------------------------
class rxDirectShow
{
	IEnumMoniker *m_pClassEnum;
	IMoniker *m_pMoniker;

	IMediaControl *m_pMC;		// ���f�B�A�R���g���[��
	ISampleGrabber *m_pGrab;
	BITMAPINFO m_BitmapInfo;

	ULONG m_cFetched;
	HRESULT m_HResult;
	bool m_bInit;

public:
	//! �R���X�g���N�^
	rxDirectShow()
	{
		m_bInit = true;
		CoInitialize(NULL);	// COM�̏�����

		m_pMC = 0;
		m_pGrab = 0;
		m_pClassEnum = 0;
		m_pMoniker = 0;

		//
		// �L���v�`���t�B���^�̏���
		// 

		// �L���v�`���f�o�C�X��T��
		// �f�o�C�X�񋓎q���쐬
		ICreateDevEnum *dev_enum;
		CoCreateInstance(CLSID_SystemDeviceEnum, NULL, CLSCTX_INPROC,IID_ICreateDevEnum, (void**)&dev_enum);

		// �r�f�I�L���v�`���f�o�C�X�񋓎q���쐬
		dev_enum->CreateClassEnumerator(CLSID_VideoInputDeviceCategory, &m_pClassEnum, 0);
		if(m_pClassEnum == NULL){
			printf("�r�f�I�L���v�`���f�o�C�X�͌�����܂���ł���\n");
			CoUninitialize();
			m_bInit = false;
		}

		dev_enum->Release();
	}

	//! �f�X�g���N�^
	~rxDirectShow()
	{
		// �C���^�[�t�F�[�X�̃����[�X
		if(m_pMC) m_pMC->Release();     
		if(m_pGrab) m_pGrab->Release();
		if(m_pClassEnum) m_pClassEnum->Release();
		if(m_pMoniker) m_pMoniker->Release();

		// COM�̃����[�X
		CoUninitialize();
	}

	//! �f�o�C�X���X�g�̎擾
	void GetDeviceList(vector<string> &list)
	{
		// ���X�g�N���A
		list.clear();

		if(m_pClassEnum == NULL) return;

		// EnumMoniker��Reset����
		m_pClassEnum->Reset();

		// �f�o�C�X����
		while(m_pClassEnum->Next(1, &m_pMoniker, &m_cFetched) == S_OK){
			IPropertyBag *pPropertyBag;
			char devname[256];

			// IPropertyBag��bind����
			m_pMoniker->BindToStorage(0, 0, IID_IPropertyBag, (void**)&pPropertyBag);

			VARIANT var;

			// FriendlyName���擾
			var.vt = VT_BSTR;
			pPropertyBag->Read(L"FriendlyName", &var, 0);
			WideCharToMultiByte(CP_ACP, 0, var.bstrVal, -1, devname, sizeof(devname), 0, 0);
			VariantClear(&var);

			list.push_back(devname);
		}
	}

	// ������
	void Init(int dev = 0)
	{
		if(m_pClassEnum == NULL) return;

		// EnumMoniker��Reset����
		m_pClassEnum->Reset();

		// �f�o�C�X����
		int i = 0;
		while(m_pClassEnum->Next(1, &m_pMoniker, &m_cFetched) == S_OK){
			if(i == dev) break;
			i++;
		}

		// Monkier��Filter��Bind����
		IBaseFilter *cfilter;		// �L���v�`���t�B���^
		m_pMoniker->BindToObject(0, 0, IID_IBaseFilter, (void**)&cfilter);


		//
		// �t�B���^�O���t�̏���
		//
	
		// �t�B���^�O���t�����A�C���^�[�t�F�[�X�𓾂�
		IGraphBuilder *fgraph;	// �t�B���^�O���t
		CoCreateInstance(CLSID_FilterGraph, NULL, CLSCTX_INPROC, IID_IGraphBuilder, (void**)&fgraph);
		fgraph->QueryInterface(IID_IMediaControl, (LPVOID *) &m_pMC);

		// �L���v�`���t�B���^���t�B���^�O���t�ɒǉ�
		fgraph->AddFilter(cfilter, L"Video Capture");


		//		
		// �O���o�t�B���^�̏���
		//
		AM_MEDIA_TYPE amt;

		// �O���o�t�B���^�����
		IBaseFilter *gfilter;			// �T���v���O���o�t�B���^
		CoCreateInstance(CLSID_SampleGrabber, NULL, CLSCTX_INPROC_SERVER, IID_IBaseFilter, (LPVOID*)&gfilter);
		gfilter->QueryInterface(IID_ISampleGrabber, (void**)&m_pGrab);

		// �O���o�t�B���^�̑}���ꏊ�̓���̂��߂̐ݒ�
		ZeroMemory(&amt, sizeof(AM_MEDIA_TYPE));
		amt.majortype  = MEDIATYPE_Video;
		amt.subtype    = MEDIASUBTYPE_RGB24;
		amt.formattype = FORMAT_VideoInfo; 
		m_pGrab->SetMediaType(&amt);

		// �O���o�t�B���^���t�B���^�O���t�ɒǉ�
		fgraph->AddFilter(gfilter, L"SamGra");


		//
		// �L���v�`���O���t�̏���
		//
		// �L���v�`���O���t�����   
		ICaptureGraphBuilder2 *cgraph;      // �L���v�`���O���t
		CoCreateInstance(CLSID_CaptureGraphBuilder2 , NULL, CLSCTX_INPROC, IID_ICaptureGraphBuilder2, (void**)&cgraph);

		// �t�B���^�O���t���L���v�`���O���t�ɑg�ݍ���
		cgraph->SetFiltergraph(fgraph);
		

		//
		// �𑜓x�̐ݒ�
		//
		IAMStreamConfig *sconfig = NULL;
		cgraph->FindInterface(&PIN_CATEGORY_CAPTURE, 0, cfilter, IID_IAMStreamConfig, (void**)&sconfig);

		// �s�����T�|�[�g����t�H�[�}�b�g�@�\�̐����擾
		int pin_count, pin_size;
		sconfig->GetNumberOfCapabilities(&pin_count, &pin_size);

		// ��ʉ𑜓x�̐ݒ�
		AM_MEDIA_TYPE *media_type;	// ���f�B�A �T���v���̃t�H�[�}�b�g�\����
		VIDEO_STREAM_CONFIG_CAPS vsconfig;				// �r�f�I �t�H�[�}�b�g�\����
		if(pin_size == sizeof(VIDEO_STREAM_CONFIG_CAPS)){
			for(int i = 0; i < pin_count; ++i){
				HRESULT hr = sconfig->GetStreamCaps(i, &media_type, reinterpret_cast<BYTE*>(&vsconfig));
				if(SUCCEEDED(hr)){
					if ((media_type->formattype == FORMAT_VideoInfo) && 
						(media_type->cbFormat >= sizeof(VIDEOINFOHEADER)) &&
						(media_type->pbFormat != NULL) ) 
					{
						// VIDEOINFOHEADER�̎擾
						hr = sconfig->GetFormat(&media_type);
						VIDEOINFOHEADER *video_info = reinterpret_cast<VIDEOINFOHEADER*>(media_type->pbFormat);
					
						// �𑜓x�̕ύX
						video_info->bmiHeader.biWidth = 1920;
						video_info->bmiHeader.biHeight = 1080;

						//RXCOUT << video_info->bmiHeader.biWidth << " x ";
						//RXCOUT << video_info->bmiHeader.biHeight << endl;

						// �L���X�g���đ��
						media_type->pbFormat = (BYTE*)video_info;

						// SetFormat��pbFormat�̕ύX��K�p
						sconfig->SetFormat(media_type);
					}
				}
			}
		}


		//
		// �����_�����O�̐ݒ�
		//
		// �L���v�`���O���t�̐ݒ�A�O���o�������_�����O�o�͂ɐݒ�
		cgraph->RenderStream(&PIN_CATEGORY_PREVIEW, &MEDIATYPE_Video, cfilter, NULL, gfilter);

		// �r�b�g�}�b�v���̎擾  
		m_pGrab->GetConnectedMediaType(&amt); 

		// �r�f�I �w�b�_�[�ւ̃|�C���^���l������B
		printf( "SampleSize = %d (byte)\n", amt.lSampleSize );
		VIDEOINFOHEADER *pVideoHeader = (VIDEOINFOHEADER*)amt.pbFormat;

		// �r�f�I �w�b�_�[�ɂ́A�r�b�g�}�b�v��񂪊܂܂��B
		// �r�b�g�}�b�v���� BITMAPINFO �\���̂ɃR�s�[����B
		ZeroMemory(&m_BitmapInfo, sizeof(m_BitmapInfo) );
		CopyMemory(&m_BitmapInfo.bmiHeader, &(pVideoHeader->bmiHeader), sizeof(BITMAPINFOHEADER));

		if(cfilter) cfilter->Release();
		if(gfilter) gfilter->Release();
		if(fgraph) fgraph->Release();
		if(cgraph) cgraph->Release();      
	}

	// �g�p�\���ǂ����₢���킹
	bool Flag(void){ return m_bInit; }

	//! �L���v�`���X�^�[�g
	void StartCapture(void)
	{
		m_pMC->Run();						// �����_�����O�J�n
		m_pGrab->SetBufferSamples(TRUE);	// �O���u�J�n
	}

	//! �L���v�`���X�g�b�v
	void StopCapture(void)
	{
		m_pMC->Stop();						// �����_�����O��~
		m_pGrab->SetBufferSamples(FALSE);	// �O���u��~
	}

	//! �t���[���摜�擾
	inline void QueryFrame(long *imageData)
	{
		m_HResult = m_pGrab->GetCurrentBuffer((long*)&(m_BitmapInfo.bmiHeader.biSizeImage), (long*)imageData);
	}

	//! �L���v�`������̏��
	int GetWidth(void){ return m_BitmapInfo.bmiHeader.biWidth; }
	int GetHeight(void){ return m_BitmapInfo.bmiHeader.biHeight; }
	int GetCount(void){ return m_BitmapInfo.bmiHeader.biBitCount; }
	int GetColor(void)
	{ 
		return (int)(m_BitmapInfo.bmiHeader.biSizeImage/(m_BitmapInfo.bmiHeader.biWidth*m_BitmapInfo.bmiHeader.biHeight));
	}
};