﻿/*
	brief the Sph method
 */

#include "sph_system.h"
#include "sph_header.h"

SPHSystem::SPHSystem()
{
	count = 0;
	max_particle=30000;
	num_particle=0;

	kernel=0.04f;
	mass=0.02f;

	world_size.x=0.64f;
	world_size.y=0.64f;
	world_size.z=0.64f;
	cell_size=kernel;
	//size of the grid
	grid_size.x=(uint)ceil(world_size.x/cell_size);
	grid_size.y=(uint)ceil(world_size.y/cell_size);
	grid_size.z=(uint)ceil(world_size.z/cell_size);
	//total number of the cell
	tot_cell=grid_size.x*grid_size.y*grid_size.z;
	//x,y,z of the gravity
	gravity.x=6.8f; 
	gravity.y=-0.0f;
	gravity.z=0.0f;

	wall_damping=-0.5f;
	rest_density=1000.0f;
	gas_constant=1.0f;
	viscosity=6.5f;
	time_step=0.003f;
	surf_norm=6.0f;
	surf_coe=0.1f;

	//here we use the poly6
	poly6_value=315.0f/(64.0f * PI * pow(kernel, 9));;
	spiky_value=-45.0f/(PI * pow(kernel, 6));
	visco_value=45.0f/(PI * pow(kernel, 6));

	grad_poly6=-945/(32 * PI * pow(kernel, 9));
	lplc_poly6=-945/(8 * PI * pow(kernel, 9));

	kernel_2=kernel*kernel;
	self_dens=mass*poly6_value*pow(kernel, 6);
	self_lplc_color=lplc_poly6*mass*kernel_2*(0-3/4*kernel_2);

	mem=(Particle *)malloc(sizeof(Particle)*max_particle);
	cell=(Particle **)malloc(sizeof(Particle *)*tot_cell);

	sys_running=0;

	printf("Initialize SPH:\n");
	printf("World Width : %f\n", world_size.x);
	printf("World Height: %f\n", world_size.y);
	printf("World Length: %f\n", world_size.z);
	printf("Cell Size  : %f\n", cell_size);
	printf("Grid Width : %u\n", grid_size.x);
	printf("Grid Height: %u\n", grid_size.y);
	printf("Grid Length: %u\n", grid_size.z);
	printf("Total Cell : %u\n", tot_cell);
	printf("Poly6 Kernel: %f\n", poly6_value);
	printf("Spiky Kernel: %f\n", spiky_value);
	printf("Visco Kernel: %f\n", visco_value);
	printf("Self Density: %f\n", self_dens);
}

SPHSystem::~SPHSystem()
{
	free(mem);
	free(cell);
}

void SPHSystem::update()
{

	if(sys_running == 0)
	{
		return;
	}

	build_table();
	comp_dens_pres();
	comp_force_adv();
	advection();



	if(count<10)
	{
		for(uint i=0; i<num_particle; i++)
		{
		Particle *p;
		p=&(mem[i]); 
		p->previous_pos[count]= p->current_pos;
		
		}
		count++;
	}
	else
	{
		for(uint i=0; i<num_particle; i++)
		{
		Particle *p;
		p=&(mem[i]); 
		for(int index=0;index<9;index++)
			{
				p->previous_pos[index] =p->previous_pos[index+1];
			}
			p->previous_pos[9]= p->current_pos;
		}
	}
					std::cout<<"      dfasdfdfsadfsa"<<mem[8].current_pos.x<<std::endl;
					std::cout<<"      sssssssssssssssssssssssssssssss"<<mem[8].previous_pos[8].x<<std::endl;
}

void SPHSystem::draw()
{
	glPointSize(1.0f);
	glColor3f(0.2f, 0.6f, 1.0f);

	//for(uint i=0; i<num_particle; i++)
	//{
	//	glBegin(GL_LINE_STRIP);
	//	std::vector<std::vector<Particle>>::iterator k = particleTrack.begin();
	//		for(int index=0;index<9;index++)
	//		{
	//			glVertex3f(mem[i].previous_pos[index].x *sim_ratio.x+real_world_origin.x, 
	//		}
	//	glEnd();
	//}
}


//initialize the SPH system
void SPHSystem::init_system()
{
	Vec3_f pos;
	Vec3_f vel;

	vel.x=0.0f;
	vel.y=0.0f;
	vel.z=0.0f;
	
	//add particles based on the world size
//	for(pos.x=world_size.x*0.0f; pos.x<world_size.x*0.9f; pos.x+=(kernel*0.8f))
	pos.x=world_size.x*0.0f;
	
		for(pos.y=world_size.y*0.0f; pos.y<world_size.y*0.6f; pos.y+=(kernel*0.8f))
		{
			for(pos.z=world_size.z*0.0f; pos.z<world_size.z*0.8f; pos.z+=(kernel*0.8f))
			{
				add_particle(pos, vel);
			}
		}
	

	printf("Init Particle: %u\n", num_particle);
}

//set position and velocity to a particles 
void SPHSystem::add_particle(Vec3_f pos, Vec3_f vel)
{
	Particle *p=&(mem[num_particle]);

	p->id=num_particle;

	p->current_pos=pos;
	p->previous_pos[0] = pos;

	//p->previous_pos[0]=pos;
	p->vel=vel;
	//x,y,z of the acceleration
	p->acc.x=0.0f;
	p->acc.y=0.0f;
	p->acc.z=0.0f;

	p->ev.x=0.0f;
	p->ev.y=0.0f;
	p->ev.z=0.0f;

	p->dens=rest_density;
	p->pres=0.0f;

	p->next=NULL;

	num_particle++;
}

//build table based on the hash  value of each cell
void SPHSystem::build_table()
{
	Particle *p;
	uint hash;

	for(uint i=0; i<tot_cell; i++)
	{
		cell[i]=NULL;
	}

	for(uint i=0; i<num_particle; i++)
	{
		p=&(mem[i]);
		hash=calc_cell_hash(calc_cell_pos(p->current_pos));

		if(cell[hash] == NULL)
		{
			p->next=NULL;
			cell[hash]=p;
		}
		else
		{
			p->next=cell[hash];
			cell[hash]=p;
		}
	}
}

//calculate the density and pressure
void SPHSystem::comp_dens_pres()
{
	Particle *p;
	Particle *np;

	Vec3_i cell_pos;
	Vec3_i near_pos;
	uint hash;

	Vec3_f rel_pos;
	float r2;

	for(uint i=0; i<num_particle; i++)
	{
		p=&(mem[i]); 
		cell_pos=calc_cell_pos(p->current_pos);

		p->dens=0.0f;
		p->pres=0.0f;
		//find all the particles near the particle
		for(int x=-1; x<=1; x++)
		{
			for(int y=-1; y<=1; y++)
			{
				for(int z=-1; z<=1; z++)
				{
					near_pos.x=cell_pos.x+x;
					near_pos.y=cell_pos.y+y;
					near_pos.z=cell_pos.z+z;
					hash=calc_cell_hash(near_pos);

					if(hash == 0xffffffff)
					{
						continue;
					}

					np=cell[hash];
					while(np != NULL)
					{
						rel_pos.x=np->current_pos.x-p->current_pos.x;
						rel_pos.y=np->current_pos.y-p->current_pos.y;
						rel_pos.z=np->current_pos.z-p->current_pos.z;
						r2=rel_pos.x*rel_pos.x+rel_pos.y*rel_pos.y+rel_pos.z*rel_pos.z;

						if(r2<INF || r2>=kernel_2)
						{
							np=np->next;
							continue;
						}

						p->dens=p->dens + mass * poly6_value * pow(kernel_2-r2, 3);

						np=np->next;
					}
				}
			}
		}

		p->dens=p->dens+self_dens;
		p->pres=(pow(p->dens / rest_density, 7) - 1) *gas_constant;
	}
}

//calculte forces 
void SPHSystem::comp_force_adv()
{
	Particle *p;
	Particle *np;

	Vec3_i cell_pos;
	Vec3_i near_pos;
	uint hash;

	Vec3_f rel_pos;
	Vec3_f rel_vel;

	float r2;
	float r;
	float kernel_r;
	float V;

	float pres_kernel;
	float visc_kernel;
	float temp_force;

	Vec3_f grad_color;
	float lplc_color;

	for(uint i=0; i<num_particle; i++)
	{
		p=&(mem[i]); 
		cell_pos=calc_cell_pos(p->current_pos);

		p->acc.x=0.0f;
		p->acc.y=0.0f;
		p->acc.z=0.0f;

		grad_color.x=0.0f;
		grad_color.y=0.0f;
		grad_color.z=0.0f;
		lplc_color=0.0f;
		
		for(int x=-1; x<=1; x++)
		{
			for(int y=-1; y<=1; y++)
			{
				for(int z=-1; z<=1; z++)
				{
					near_pos.x=cell_pos.x+x;
					near_pos.y=cell_pos.y+y;
					near_pos.z=cell_pos.z+z;
					hash=calc_cell_hash(near_pos);

					if(hash == 0xffffffff)
					{
						continue;
					}

					np=cell[hash];
					while(np != NULL)
					{
						rel_pos.x=p->current_pos.x-np->current_pos.x;
						rel_pos.y=p->current_pos.y-np->current_pos.y;
						rel_pos.z=p->current_pos.z-np->current_pos.z;
						r2=rel_pos.x*rel_pos.x+rel_pos.y*rel_pos.y+rel_pos.z*rel_pos.z;

						if(r2 < kernel_2 && r2 > INF)
						{
							r=sqrt(r2);
							V=mass/np->dens/2;
							kernel_r=kernel-r;

							pres_kernel=spiky_value * kernel_r * kernel_r;
							temp_force=V * (p->pres+np->pres) * pres_kernel;
							p->acc.x=p->acc.x-rel_pos.x*temp_force/r;
							p->acc.y=p->acc.y-rel_pos.y*temp_force/r;
							p->acc.z=p->acc.z-rel_pos.z*temp_force/r;

							rel_vel.x=np->ev.x-p->ev.x;
							rel_vel.y=np->ev.y-p->ev.y;
							rel_vel.z=np->ev.z-p->ev.z;

							visc_kernel=visco_value*(kernel-r);
							temp_force=V * viscosity * visc_kernel;
							p->acc.x=p->acc.x + rel_vel.x*temp_force; 
							p->acc.y=p->acc.y + rel_vel.y*temp_force; 
							p->acc.z=p->acc.z + rel_vel.z*temp_force; 

							float temp=(-1) * grad_poly6 * V * pow(kernel_2-r2, 2);
							grad_color.x += temp * rel_pos.x;
							grad_color.y += temp * rel_pos.y;
							grad_color.z += temp * rel_pos.z;
							lplc_color += lplc_poly6 * V * (kernel_2-r2) * (r2-3/4*(kernel_2-r2));
						}

						np=np->next;
					}
				}
			}
		}

		lplc_color+=self_lplc_color/p->dens;
		p->surf_norm=sqrt(grad_color.x*grad_color.x+grad_color.y*grad_color.y+grad_color.z*grad_color.z);

		if(p->surf_norm > surf_norm)
		{
			p->acc.x+=surf_coe * lplc_color * grad_color.x / p->surf_norm;
			p->acc.y+=surf_coe * lplc_color * grad_color.y / p->surf_norm;
			p->acc.z+=surf_coe * lplc_color * grad_color.z / p->surf_norm;
		}
	}
}

//calculate the advection of the fluid 移流项
void SPHSystem::advection()
{
	Particle *p;
	Vec3 wall_normal;
	wall_normal.data[0] =-1.0;
	for(uint i=0; i<num_particle; i++)
	{
		p=&(mem[i]);

		//float m_dot = dot(wall_normal, (p->pos-Vec3_f(0.6,0.325,0.34)));

		p->vel.x=p->vel.x+p->acc.x*time_step/p->dens+gravity.x*time_step;
		p->vel.y=p->vel.y+p->acc.y*time_step/p->dens+gravity.y*time_step;
		p->vel.z=p->vel.z+p->acc.z*time_step/p->dens+gravity.z*time_step;

		p->current_pos.x=p->current_pos.x+p->vel.x*time_step;
		p->current_pos.y=p->current_pos.y+p->vel.y*time_step;
		p->current_pos.z=p->current_pos.z+p->vel.z*time_step;
	


		//if(p->pos.x >= world_size.x-BOUNDARY)
		//{
		//	p->vel.x=p->vel.x*wall_damping;
		//	p->pos.x=world_size.x-BOUNDARY;
		//}
		Vec3 impulse = normalize(wall_normal);
		impulse *= (1.0f+0.0f)*dot(wall_normal, Vec3(p->vel.x,p->vel.y,p->vel.z));

		if(p->current_pos.x >=0.64)
		{
			if(p->current_pos.y>0.25&&p->current_pos.y<0.4&&p->current_pos.z>0.25&&p->current_pos.z<0.43)
			{
			  p->vel.x=p->vel.x*wall_damping;
				//p->vel.x = -1*(float)impulse.data[0];
				//p->vel.y = -1*(float)impulse.data[1];
				//p->vel.z = -1*(float)impulse.data[2];
			p->current_pos.x=0.6;
			}
		}

		if(p->current_pos.x < 0.0f)
		{
			p->vel.x=p->vel.x*wall_damping;
			p->current_pos.x=0.0f;
		}

		if(p->current_pos.y >= world_size.y-BOUNDARY)
		{
			p->vel.y=p->vel.y*wall_damping;
			p->current_pos.y=world_size.y-BOUNDARY;
		}

		if(p->current_pos.y < 0.0f)
		{
			p->vel.y=p->vel.y*wall_damping;
			p->current_pos.y=0.0f;
		}

		if(p->current_pos.z >= world_size.z-BOUNDARY)
		{
			p->vel.z=p->vel.z*wall_damping;
			p->current_pos.z=world_size.z-BOUNDARY;
		}

		if(p->current_pos.z < 0.0f)
		{
			p->vel.z=p->vel.z*wall_damping;
			p->current_pos.z=0.0f;
		}


		p->ev.x=(p->ev.x+p->vel.x)/2;
		p->ev.y=(p->ev.y+p->vel.y)/2;
		p->ev.z=(p->ev.z+p->vel.z)/2;
	}

}

Vec3_i SPHSystem::calc_cell_pos(Vec3_f p)
{
	Vec3_i cell_pos;
	cell_pos.x = int(floor((p.x) / cell_size));
	cell_pos.y = int(floor((p.y) / cell_size));
	cell_pos.z = int(floor((p.z) / cell_size));

    return cell_pos;
}

uint SPHSystem::calc_cell_hash(Vec3_i cell_pos)
{
	if(cell_pos.x<0 || cell_pos.x>=(int)grid_size.x || cell_pos.y<0 || cell_pos.y>=(int)grid_size.y || cell_pos.z<0 || cell_pos.z>=(int)grid_size.z)
	{
		return (uint)0xffffffff;
	}

	cell_pos.x = cell_pos.x & (grid_size.x-1);  
    cell_pos.y = cell_pos.y & (grid_size.y-1);  
	cell_pos.z = cell_pos.z & (grid_size.z-1);  

	return ((uint)(cell_pos.z))*grid_size.y*grid_size.x + ((uint)(cell_pos.y))*grid_size.x + (uint)(cell_pos.x);
}
