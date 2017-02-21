/*! 
  @file rx_webp.h
	
  @brief WebP�摜�t�@�C���ǂݍ��݁C�����o��
	- libwebp : http://code.google.com/intl/ja/speed/webp/
 
  @author Makoto Fujisawa
  @date 2011-11
*/
// FILE --rx_webp.h--

#ifndef _RX_WEBP_H_
#define _RX_WEBP_H_


//-----------------------------------------------------------------------------
// �C���N���[�h�t�@�C��
//-----------------------------------------------------------------------------
#include <cstdio>
#include <string>

#include "webp/decode.h"
#include "webp/encode.h"


//-----------------------------------------------------------------------------
// WebP�摜�t�@�C���̓ǂݍ���/��������
//-----------------------------------------------------------------------------
/*!
 * WebP�t�@�C���̓ǂݍ���
 * @param[in] fn �t�@�C����
 * @param[out] w,h �摜�T�C�Y
 * @param[out] c �摜�̐F�[�x
 * @return �W�J�ς݉摜�f�[�^
 */
static unsigned char* ReadWebPFile(const std::string &fn, int &w, int &h, int &c)
{
	WebPDecoderConfig config;
	WebPDecBuffer* const output_buffer = &config.output;
	WebPBitstreamFeatures* const bitstream = &config.input;

	// �f�R�[�h�ݒ��������
	if(!WebPInitDecoderConfig(&config)){
		fprintf(stderr, "WebP error : library version mismatch.\n");
		return 0;
	}

	VP8StatusCode status = VP8_STATUS_OK;
	uint32_t data_size = 0;
	void* data = 0;
	FILE *fp = 0;
	unsigned char* img = 0;

	try{
		// �t�@�C�����o�C�i�����[�h�ŊJ��
		if((fp = fopen(fn.c_str(), "rb")) == NULL){
			throw("WebP error : cannot open %s file.", fn.c_str());
		}

		fseek(fp, 0, SEEK_END);
		data_size = ftell(fp);
		fseek(fp, 0, SEEK_SET);
		data = malloc(data_size);
		if(fread(data, data_size, 1, fp) != 1){
			throw("WebP error : could not read %d bytes of data from file %s .", data_size, fn.c_str());
		}

		// �r�b�g�X�g���[���̓������擾
		status = WebPGetFeatures((const uint8_t*)data, data_size, bitstream);
		if(status == VP8_STATUS_OK){
			// �f�R�[�h�ݒ��K�v�ɉ����ďC��
			config.output.colorspace = MODE_RGBA;

			// �f�R�[�h
			status = WebPDecode((const uint8_t*)data, data_size, &config);
		}
		else{
			throw("WebP error : decoding of %s failed.", fn.c_str());
		}

		// �o�̓f�[�^
		w = output_buffer->width;
		h = output_buffer->height;
		c = (bitstream->has_alpha ? 4 : 3);

		const unsigned char* const rgb = output_buffer->u.RGBA.rgba;
		int size = output_buffer->u.RGBA.size;

		// �o�̓f�[�^�փR�s�[
		img = new unsigned char[w*h*c];
		for(int i = 0; i < size; ++i){
			img[i] = rgb[i];
		}
	}
	catch(const char* str){
		fprintf(stderr, "%s\n", str);
	}

	if(fp) fclose(fp);
	if(data) free(data);

	// �f�R�[�_�����
	WebPFreeDecBuffer(output_buffer);

	return img;
}

//! ���k�ݒ�
struct rxWebPConfig
{
	//! �v���Z�b�g(default, photo, picture, drawing, icon, text)�C�v���Z�b�g���w�肳��Ă�����ȉ��̐ݒ�͖��������
	string preset;	

	// �e��ݒ�
	int method;				//!< ���k���@[0,6] (0:fast, 6:slowest)
	int target_size;		//!< �^�[�Q�b�g�T�C�Y (byte)
	float target_PSNR;		//!< �^�[�Q�b�gPSNR (dB, �ʏ�42���炢)
	int sns_strength;		//!< Spatial Noise Shaping [0,100]
	int filter_strength;	//!< �t�B���^���x[0,100] (0�Ńt�B���^OFF)
	bool autofilter;		//!< �����t�B���^�ݒ�
	int filter_sharpness;	//!< �V���[�v�l�X�t�B���^[0,7] (0:most, 7:least sharp)
	bool strong;			//!< simple�̑����strong�t�B���^��p����
	int pass;				//!< ��̓p�X��[1,10]
	int segments;			//!< �Z�O�����g��[1,4]
	int partition_limit;	//!< �ŏ��̃p�[�e�B�V������512k�Ƀt�B�b�g����悤�ɉ掿�𐧌�[0,100] (100��full)
	int alpha_compression;	//!< �����x(�A���t�@�l)�̈��k��ݒ�

	rxWebPConfig()
	{
		preset.clear();
		method = -1;
		target_size = -1;
		target_PSNR = -1;
		sns_strength = -1;
		filter_strength = -1;
		filter_sharpness = -1;
		autofilter = false;
		strong = false;
		pass = -1;
		segments = -1;
		partition_limit = -1;
		alpha_compression = -1;
	}
};

//! �G���[���b�Z�[�W
static const char* RX_WEBP_ERROR_MESSAGES[] = 
{
	"OK",
	"OUT_OF_MEMORY: Out of memory allocating objects",
	"BITSTREAM_OUT_OF_MEMORY: Out of memory re-allocating byte buffer",
	"NULL_PARAMETER: NULL parameter passed to function",
	"INVALID_CONFIGURATION: configuration is invalid",
	"BAD_DIMENSION: Bad picture dimension. Maximum width and height is 16383 pixels.", 
	"PARTITION0_OVERFLOW: Partition #0 is too big to fit 512k.", 
	"PARTITION_OVERFLOW: Partition is too big to fit 16M",
	"BAD_WRITE: Picture writer returned an I/O error"
};

//! �摜��������
static int RxWebPWriter(const uint8_t* data, size_t data_size, const WebPPicture* const pic)
{
	FILE* const out = (FILE*)pic->custom_ptr;
	return data_size ? (fwrite(data, data_size, 1, out) == 1) : 1;
}


/*!
 * WebP�t�@�C���̏�������
 * @param[in] fn �t�@�C����
 * @param[in] img �摜�f�[�^
 * @param[in] w,h �摜�T�C�Y
 * @param[in] c �摜�̐F�[�x
 * @param[in] quality ���k�i��[0,100]
 * @param[in] rxcfg ���k�ݒ�
 */
static int WriteWebPFile(const std::string &fn, unsigned char *img, int w, int h, int c, int q, rxWebPConfig rxcfg = rxWebPConfig())
{
	WebPPicture picture;
	WebPConfig config;
	WebPAuxStats stats;
	FILE *fp = 0;
	int rtn = 1;

	try{
		// ���̓f�[�^�̏�����
		if(!WebPPictureInit(&picture) || !WebPConfigInit(&config)){
			throw("WebP error : version mismatch.");
		}

		picture.width = w;
		picture.height = h;
		int stride = c*w*sizeof(*img);
		if(c == 4){
			WebPPictureImportRGBA(&picture, img, stride);
		}
		else{
			WebPPictureImportRGB(&picture, img, stride);
		}

		config.quality = (float)q;

		if(rxcfg.preset.empty()){
			if(rxcfg.method >= 0) config.method = rxcfg.method;
			if(rxcfg.target_size >= 0) config.target_size = rxcfg.target_size;
			if(rxcfg.target_PSNR >= 0) config.target_PSNR = rxcfg.target_PSNR;
			if(rxcfg.sns_strength >= 0) config.sns_strength = rxcfg.sns_strength;
			if(rxcfg.filter_strength >= 0) config.filter_strength = rxcfg.filter_strength;
			if(rxcfg.method >= 0) config.filter_sharpness = rxcfg.filter_sharpness;
			if(rxcfg.autofilter) config.autofilter = 1;
			if(rxcfg.strong) config.filter_type = 1;
			if(rxcfg.pass >= 0) config.pass = rxcfg.pass;
			if(rxcfg.segments >= 0) config.segments = rxcfg.segments;
			if(rxcfg.partition_limit >= 0) config.partition_limit = rxcfg.partition_limit;
			if(rxcfg.alpha_compression >= 0) config.alpha_compression = rxcfg.alpha_compression;
		}
		else{
			WebPPreset preset;
			if(rxcfg.preset == "default"){
				preset = WEBP_PRESET_DEFAULT;
			}
			else if(rxcfg.preset == "photo"){
				preset = WEBP_PRESET_PHOTO;
			}
			else if(rxcfg.preset == "picture"){
				preset = WEBP_PRESET_PICTURE;
			}
			else if(rxcfg.preset == "drawing"){
				preset = WEBP_PRESET_DRAWING;
			}
			else if(rxcfg.preset == "icon"){
				preset = WEBP_PRESET_ICON;
			}
			else if(rxcfg.preset == "text"){
				preset = WEBP_PRESET_TEXT;
			}
			else{
				preset = WEBP_PRESET_DEFAULT;
			}

			// �v���Z�b�g��ݒ�
			if(!WebPConfigPreset(&config, preset, config.quality)){
				throw("WebP error : preset error.");
			}
		}

		// �o�͐ݒ������
		if(!WebPValidateConfig(&config)){
			throw("WebP error : invalid configuration.");
		}

		// �t�@�C�����o�C�i�����[�h�ŊJ��
		if((fp = fopen(fn.c_str(), "wb")) == NULL){
			throw("WebP error : cannot open %s file", fn.c_str());
		}

		picture.writer = RxWebPWriter;
		picture.custom_ptr = (void*)fp;
		picture.stats = &stats;

		// ���k
		if(!WebPEncode(&config, &picture)){
			throw("WebP error : can't encode picture (code : %d - %s).", 
				  picture.error_code, RX_WEBP_ERROR_MESSAGES[picture.error_code]);
		}
	}
	catch(const char* str){
		fprintf(stderr, "%s\n", str);
		rtn = 0;
	}

	free(picture.extra_info);
	if(fp) fclose(fp);

	// �G���R�[�_�����
	WebPPictureFree(&picture);
	
	return rtn;
}


#endif // #ifdef _RX_WEBP_H_
