//-----------------------------------------------------------------------------------
// �}�N��-------------string
//-----------------------------------------------------------------------------------
//�����֘A
#define LINE_SP 40							//�����̕�����
#define LINE_K 10.0*((double)LINE_SP)		//�΂˒萔
#define LINE_D 1.0							//�����W��
#define LINE_RHO 0.2						//���ʖ��x
#define LINE_E 0.02							//��R�W��
#define SEG_LENGTH 0.1f						//the initial length of two particles

#define RX_GOUND_HEIGHT  0.0		//ground height

//-----------------------------------------------------------------------------------
// �}�N��----------------kite
//-----------------------------------------------------------------------------------
//���֘A



#define COL_NUM 13							//���̉����������_��->(4�̔{��+1)
#define ROW_NUM 13							//���̏c���������_��

#define RHO 1.21							//��C���x(1.2kg/m^3)
#define G_ACCE -9.8							//�d�͉����x

#define KITE_RHO 0.2						//���̖��x? ��{�I�ɂ͈����₷���悤200g/1m^2

//�e�[�u���֌W
#define TABLE_S 0.68						//�A�X�y�N�g��
#define TABLE_L 1.48						//�A�X�y�N�g��

//�����ۊ֘A
#define TAIL_NUM 2							//�����ۂ̐�

#define TAIL_SP 10							//�����ۂ̕�����
#define TAIL_K 10.0*((double)TAIL_SP)		//�΂˒萔
#define TAIL_D 1.0							//�����W��
#define TAIL_RHO 0.2						//���ʖ��x
#define TAIL_E 0.02							//��R�W��

//����݊֘A
#define BAR_SP (COL_NUM-1)					//���̕�����
#define BAR_L 1.0							//���̒���
#define BAR_WIDTH 0.016						//���f�ʂ̕�(�����`�f�ʂ̏ꍇ,���������˂�)
											//(���悻0.02m�قǂ���͂�����ω����킩��)
#define INV_BAMBOO_E pow(10.0,-9.0)			//�|�̃����O���̋t��

//-----------------------------------------------------------------------------------
// �}�N��----fluid
//-----------------------------------------------------------------------------------
#define IX(i,j,k) ((i)+(N+2)*(j)+(N+2)*(N+2)*(k))//!< 3�����̊i�[
#define SWAP(x0,x) {double * tmp=x0;x0=x;x=tmp;}//!< �z��̓���ւ�

#define GRID 16			//!< 1�������̃O���b�h������
#define F_FORCE -1.0	//!< �O��
#define STEP 0.01		//!< �^�C���X�e�b�v��
#define VISC 0.0007		//!< ���S���W��
#define DIFF 0.0		//!< �g�U�W��


