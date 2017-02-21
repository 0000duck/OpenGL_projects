/*! 
  @file rx_png.h
	
  @brief PNG�t�@�C���ǂݍ��݁C�����o��
	- libpng : http://www.libpng.org/pub/png/libpng.html
	- zlib : http://www.zlib.net/
 
  @author Makoto Fujisawa
  @date 2011-06
*/
// FILE --rx_png.h--

#ifndef _RX_PNG_H_
#define _RX_PNG_H_



//-----------------------------------------------------------------------------
// �C���N���[�h�t�@�C��
//-----------------------------------------------------------------------------
#include <cstdio>
#include <string>

#include <png.h>


//-----------------------------------------------------------------------------
// PNG�t�@�C���̓ǂݍ��݂Ə�������
//-----------------------------------------------------------------------------
inline void ReadPngCallback(png_structp png_ptr, png_uint_32 row_number, int pass)
{
	
}

/*!
 * PNG�t�@�C���̓ǂݍ���
 * @param[in] fn �t�@�C����
 * @param[out] w,h �摜�T�C�Y
 * @param[out] c �摜�̐F�[�x
 * @return �W�J�ς݉摜�f�[�^
 */
static unsigned char* ReadPngFile(const std::string &fn, int &w, int &h, int &c)
{
	png_FILE_p fp;
	unsigned char* img = 0;	// �o�̓f�[�^

	// �t�@�C�����o�C�i�����[�h�ŊJ��
	if((fp = fopen(fn.c_str(), "rb")) == NULL){
		fprintf(stderr, "png error : cannot open %s file\n", fn.c_str());
		return 0;
	}

	png_structp read_ptr;
	png_infop read_info_ptr;

	// PNG�ǂݍ��݃I�u�W�F�N�g����
	read_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	read_info_ptr = png_create_info_struct(read_ptr);
	
	// �ǂݍ��ރf�[�^��o�^
	png_init_io(read_ptr, fp);

	// �ǂݍ��ݎ��̃R�[���o�b�N�̐ݒ�(�ǂݍ��ݏ󋵂�\���������ꍇ�Ȃ�)
	png_set_read_status_fn(read_ptr, NULL);

	// �w�b�_���̓ǂݍ���
	png_read_info(read_ptr, read_info_ptr);

	// �摜���̎擾
	png_uint_32 width, height;
	int bit_depth, color_type;
	int interlace_type, compression_type, filter_type;

	if(png_get_IHDR(read_ptr, read_info_ptr, &width, &height, &bit_depth,
					&color_type, &interlace_type, &compression_type, &filter_type))
	{
		w = (int)width;
		h = (int)height;
		c = 0;

		// �`�����l����
		c += (color_type & PNG_COLOR_MASK_COLOR) ? 3 : 0;
		c += (color_type & PNG_COLOR_MASK_ALPHA) ? 1 : 0;
		
		if(c && (color_type & PNG_COLOR_MASK_PALETTE)){
			png_set_palette_to_rgb(read_ptr);
			c = 4;
		}
		else if(!c){	// color_type == PNG_COLOR_TYPE_GRAY
			c = 1;
		}

		if(!c){
			png_destroy_read_struct(&read_ptr, &read_info_ptr, NULL);
			fclose(fp);
			return 0;
		}

		// �e�s�ւ̃|�C���^���g���ĉ摜��ǂݍ���
		img = new unsigned char[w*h*c];
		unsigned char **lines = new unsigned char*[h];
		for(int j = 0; j < h; ++j) lines[j] = &img[j*w*c];

		png_read_image(read_ptr, lines);

		delete [] lines;
	}
	
	png_destroy_read_struct(&read_ptr, &read_info_ptr, NULL);
	fclose(fp);

	return img;
}

/*!
 * PNG�t�@�C���̏�������
 * @param[in] fn �t�@�C����
 * @param[in] img �摜�f�[�^
 * @param[in] w,h �摜�T�C�Y
 * @param[in] c �摜�̐F�[�x
 * @param[in] quality ���k�i��[0,100]
 */
static int WritePngFile(const std::string &fn, unsigned char *img, int w, int h, int c)
{
	png_FILE_p fp;

	// �t�@�C�����o�C�i�����[�h�ŊJ��
	if((fp = fopen(fn.c_str(), "wb")) == NULL){
		fprintf(stderr, "png error : cannot open %s file\n", fn.c_str());
		return 0;
	}

	png_structp write_ptr;
	png_infop write_info_ptr;

	// PNG�ǂݍ��݃I�u�W�F�N�g����
	write_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	write_info_ptr = png_create_info_struct(write_ptr);

	if(setjmp(png_jmpbuf(write_ptr))){
		fclose(fp);
		return 0;
	}
	
	// �������ރt�@�C����o�^
	png_init_io(write_ptr, fp);

	// �ǂݍ��ݎ��̃R�[���o�b�N�̐ݒ�(�ǂݍ��ݏ󋵂�\���������ꍇ�Ȃ�)
	png_set_write_status_fn(write_ptr, NULL);

	// �J���[�^�C�v
	int color_type = 0;
	if(c == 1) color_type = PNG_COLOR_TYPE_GRAY;
	else if(c == 3) color_type = PNG_COLOR_TYPE_RGB;
	else if(c == 4) color_type = PNG_COLOR_TYPE_RGB_ALPHA;
	
	// �w�b�_���̓o�^
	png_set_IHDR(write_ptr, write_info_ptr, w, h, 8, color_type, 
				 PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_BASE);

	png_color_8 sig_bit;
	sig_bit.red = 8;
	sig_bit.green = 8;
	sig_bit.blue = 8;
	sig_bit.alpha = 0;
	png_set_sBIT(write_ptr, write_info_ptr, &sig_bit);

	// PNG�ɏ������܂��R�����g
	png_text text_ptr[1];
	text_ptr[0].key = "Description";
	text_ptr[0].text = "Saved by libpng";
	text_ptr[0].compression = PNG_TEXT_COMPRESSION_NONE;
	png_set_text(write_ptr, write_info_ptr, text_ptr, 1);

	// �w�b�_���̏�������
	png_write_info(write_ptr, write_info_ptr);

	//png_set_bgr(write_ptr);

	// �e�s�ւ̃|�C���^���g���ĉ摜��ǂݍ���
	unsigned char **lines = new unsigned char*[h];
	for(int j = 0; j < h; ++j) lines[j] = &img[j*w*c];

	png_write_image(write_ptr, lines);

	delete [] lines;

	png_write_end(write_ptr, write_info_ptr);
	png_destroy_write_struct(&write_ptr, &write_info_ptr);
	fclose(fp);

	return 1;
}




#endif // #ifdef _RX_PNG_H_
