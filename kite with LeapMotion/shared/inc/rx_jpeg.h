/*! 
  @file rx_jpeg.h
	
  @brief JPEG�t�@�C���ǂݍ��݁C�����o��
	- libjpeg���g�p
	- http://www.ijg.org/
 
  @author Makoto Fujisawa
  @date 2011-06
*/
// FILE --rx_jpeg.h--

#ifndef _RX_JPEG_H_
#define _RX_JPEG_H_


//-----------------------------------------------------------------------------
// �C���N���[�h�t�@�C��
//-----------------------------------------------------------------------------
#include <cstdio>

#include <vector>
#include <string>

#include <jpeglib.h>
#include <jerror.h>


//-----------------------------------------------------------------------------
// JPEG�t�@�C���̓ǂݍ��݂Ə�������
//-----------------------------------------------------------------------------
/*!
 * JPEG�t�@�C���̓ǂݍ���
 * @param[in] fn �t�@�C����
 * @param[out] w,h �摜�T�C�Y
 * @param[out] c �摜�̐F�[�x
 * @return �W�J�ς݉摜�f�[�^
 */
static unsigned char* ReadJpegFile(const std::string &fn, int &w, int &h, int &c)
{
	jpeg_decompress_struct cinfo;
	jpeg_error_mgr jerr;

	// JPEG�𓀗p�I�u�W�F�N�g����
	cinfo.err = jpeg_std_error(&jerr);
	jpeg_create_decompress(&cinfo);

	// �t�@�C�����o�C�i�����[�h�ŊJ��
	FILE *fp;
	if((fp = fopen(fn.c_str(), "rb")) == NULL){
		fprintf(stderr, "jpeg error : cannot open %s file\n", fn.c_str());
		return 0;
	}

	// �𓀂���f�[�^���w��
	jpeg_stdio_src(&cinfo, fp);

	// �t�@�C���w�b�_�̓ǂݍ���
	jpeg_read_header(&cinfo, TRUE);

	// �摜�F�[�x
	c = cinfo.num_components;
	if(!c){
		fprintf(stderr, "jpeg error : the number of color components is zero\n");
		return 0;
	}

	// �摜�T�C�Y
	w = cinfo.image_width;
	h = cinfo.image_height;
	if(!w || !h){
		fprintf(stderr, "jpeg error : size of the image is zero\n");
		return 0;
	}

	// �f�[�^����
	jpeg_start_decompress(&cinfo);

	// �f�[�^
	JSAMPARRAY buf;
	buf = (*cinfo.mem->alloc_sarray)((j_common_ptr)&cinfo, JPOOL_IMAGE, w*c, (JDIMENSION)1);

	// �o�̓f�[�^
	unsigned char* img = new unsigned char[w*h*c];

	// 1�s���R�s�[
	unsigned char* dst_ptr = img;
	unsigned char* src_ptr;
	while(cinfo.output_scanline < (unsigned int)h){
		jpeg_read_scanlines(&cinfo, buf, 1);
		src_ptr = buf[0];
		for(int i = 0; i < w*c; ++i){
			*dst_ptr++ = *src_ptr++;
		}
	}
	
	jpeg_finish_decompress(&cinfo);
	jpeg_destroy_decompress(&cinfo);
	fclose(fp);

	return img;
}

/*!
 * JPEG�t�@�C���̏�������
 * @param[in] fn �t�@�C����
 * @param[in] img �摜�f�[�^
 * @param[in] w,h �摜�T�C�Y
 * @param[in] c �摜�̐F�[�x
 * @param[in] quality ���k�i��[0,100]
 */
static int WriteJpegFile(const std::string &fn, unsigned char *img, int w, int h, int c, int quality)
{
	jpeg_compress_struct cinfo;
	jpeg_error_mgr jerr;

	// JPEG���k�p�I�u�W�F�N�g����
	cinfo.err = jpeg_std_error(&jerr);
	jpeg_create_compress(&cinfo);

	// �t�@�C�����o�C�i�����[�h�ŊJ��
	FILE *fp;
	if((fp = fopen(fn.c_str(), "wb")) == NULL){
		fprintf(stderr, "jpeg error : cannot open %s file\n", fn.c_str());
		return 0;
	}

	// �o�̓t�@�C�����w��
	jpeg_stdio_dest(&cinfo, fp);

	// �摜�t�@�C�����
	cinfo.image_width = w;
	cinfo.image_height = h;
	cinfo.input_components = c;
	cinfo.in_color_space = (c == 3 ? JCS_RGB : JCS_GRAYSCALE);

	// ���k�ݒ�
	jpeg_set_defaults(&cinfo);	// �f�t�H���g�̃p�����[�^���Z�b�g
	jpeg_set_quality(&cinfo, quality, TRUE);	// �摜�i����ݒ�

	// �f�[�^�����k
	jpeg_start_compress(&cinfo, TRUE);

	// 1�s���o��
	unsigned char* src_ptr = img;
	JSAMPARRAY dst_ptr = new JSAMPROW[w*c];
	while(cinfo.next_scanline < (unsigned int)h){
		jpeg_write_scanlines(&cinfo, &src_ptr, 1);
		src_ptr += w*c;
	}
	
	jpeg_finish_compress(&cinfo);
	jpeg_destroy_compress(&cinfo);
	fclose(fp);

	return 1;
}




#endif // #ifdef _RX_JPEG_H_
