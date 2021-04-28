#include <cutil.h>
#include <GL/glut.h>
#include <cuda_gl_interop.h>


//#include "Kernel.cu"
#include "radixsort.cuh"

//START OF KERNEL.CU!!!!!!!!! (END AT LINE 467) 

#pragma once

#include "cutil_math.h"
#include "math_constants.h"
#include "Params.cuh"


typedef unsigned int uint;

#if USE_TEX
texture<float4, 1, cudaReadModeElementType> oldPosTex;
texture<float4, 1, cudaReadModeElementType> oldVelTex;

texture<uint2, 1, cudaReadModeElementType> particleHashTex;
texture<uint,  1, cudaReadModeElementType> cellStartTex;

texture<float, 1, cudaReadModeElementType> pressureTex;
texture<float, 1, cudaReadModeElementType> densityTex;

texture<float, 1, cudaReadModeElementType> dyeColorTex;
#endif

__constant__ SimParams par;



//----------------------------------------------------------------------------------------------------------------------------
///  Boundary Conditions
//----------------------------------------------------------------------------------------------------------------------------

__device__ void boundary(float3& pos, float3& vel)
{
	//  world box
	float3 wmin = par.worldMin, wmax = par.worldMax;

	float b = par.distBndSoft, stiff = par.bndStiff, damp = par.bndDamp, damp2 = par.bndDampC;
	float accBnd, diff;  float3 norm;
	BndType t = par.bndType;	bool bCylY = t==BND_CYL_Y, bCylZ = t==BND_CYL_Z,
		/*Eff*/ bWave = par.bndEffZ == BND_EFF_WAVE, bNoEff = par.bndEffZ == BND_EFF_NONE, bCycle = par.bndEffZ == BND_EFF_CYCLE;

	#define  EPS	0.00001f	// epsilon for collision detection
	#define  addB()  accBnd = stiff * diff - damp * dot(norm, vel);  vel += accBnd * norm * par.timeStep;	// box,pump, soft
	#define  addC()  accBnd = stiff * diff - damp2* dot(norm, vel);  vel += accBnd * norm * par.timeStep;	// cyl,sphr

	if (bWave)
	{	float sl = -par.r2Angle;
		//  slope
		diff = b - (pos.y - wmin.y) - (pos.z - wmin.z)*sl;	if (diff > EPS)  {  norm = make_float3(0, 1-sl, sl);  addB();  }
		//  shore waves
		wmin.z += par.rTwist*(1.f + sinf(par.rAngle));
	}

	//----------------  Box
	if (t != BND_SPHERE)
	{
		if (!bCylY)  {
			if (bNoEff || bWave){	diff = b - pos.z + wmin.z;	if (diff > EPS)  {	norm = make_float3( 0, 0, 1);  addC();  }	}
			if (!bCycle)	{		diff = b + pos.z - wmax.z;	if (diff > EPS)  {	norm = make_float3( 0, 0,-1);  addC();  }	}
		}
		if (!bCylY && !bCylZ)  {
			diff = b - pos.x + wmin.x;	if (diff > EPS)  {	norm = make_float3( 1, 0, 0);  addB();  }
			diff = b + pos.x - wmax.x;	if (diff > EPS)  {	norm = make_float3(-1, 0, 0);  addB();  }	}

		if (!bCylZ)  {
			diff = b - pos.y + wmin.y;	if (diff > EPS)  {	norm = make_float3( 0, 1, 0);  addB();  }
			diff = b + pos.y - wmax.y;	if (diff > EPS)  {	norm = make_float3( 0,-1, 0);  addB();  }	}
	}
	else	//  Sphere
	/*if (t == BND_SPHERE)*/  {
		float len = length(pos);	diff = b + len + wmin.y;
		if (diff > EPS)  {	norm = make_float3( -pos.x/len, -pos.y/len, -pos.z/len);  addC();  }  }

	//  Cylinder Y|
	if (bCylY || t == BND_CYL_YZ)  {
		float len = length(make_float2(pos.x, pos.z));		diff = b + len - wmax.x;
		if (diff > EPS)  {	norm = make_float3( -pos.x/len, 0, -pos.z/len);  addC();  }	}

	//  Cylinder Z--
	if (bCylZ || t == BND_CYL_YZ)  {
		float len = length(make_float2(pos.x, pos.y));		diff = b + len + wmin.y;
		if (diff > EPS)  {	norm = make_float3( -pos.x/len, -pos.y/len, 0);  addC();  }  }
	
	//  Wrap, Cycle  Z--
	if (!bWave && !bNoEff)  {		float dr = 1.f*par.particleR;/*par.rDexit*/
		if (bCycle &&
			vel.z > par.rVexit && pos.z > wmax.z -b -dr) {  pos.z -= wmax.z - wmin.z -2*b -dr;  }  else
		if (vel.z <-par.rVexit && pos.z < wmin.z +b +dr) {  pos.z += wmax.z - wmin.z -2*b -dr;  }  }


	///  Pump  ~~~~~~~~~~~~~~~~~~~~~~~~~~
	if (t == BND_PUMP_Y)
	{
		float rad = wmax.x, ang = par.angOut, hc = par.hClose, rin = rad * par.radIn;
		
		//  cyl Y|  Frame
		float len = length(make_float2(pos.x, pos.y));		diff = b + len - rad;
		if (diff > EPS)
		{	float a = atan(pos.x / pos.y);
			if (ang < 0.5f)
			{	if (a < -ang || a > ang || pos.y < 0)			// ^
				{	norm = make_float3( -pos.x/len, -pos.y/len, 0);  addB();  }	}
			else
			{	if (pos.y < 0 || (len < rad*par.s5 && a < ang))	// S
				{	norm = make_float3( -pos.x/len, -pos.y/len, 0);  addB();  }	}
		}
		
		//  box Out
		float xs;
		if (ang < 0.5f)		// ^
		{	xs = sinf(ang * par.s3)*rad;
			float zs = cosf(ang * par.s3)*rad * par.s4;
			if (pos.y > zs)
			{	diff = b - pos.x - xs;	if (diff > EPS)  {	norm = make_float3( 1, 0, 0);  addB();  }
				diff = b + pos.x - xs;	if (diff > EPS)  {	norm = make_float3(-1, 0, 0);  addB();  }
			}
		}else{	// S
			xs = 0.09f*par.s4;
			if (len >= rad*par.s6)
			{	diff = b - pos.x + xs;	if (diff > EPS)  {	norm = make_float3( 1, 0, 0);  addB();  }	}
		}
		
		//  cyl hole In  (upper close)
		if (pos.z > hc - b*par.s1)
		{	diff = b + len - rin;	if (diff > EPS)  {	norm = make_float3( -pos.x/len, -pos.y/len, 0);  addB();  }	}
		if (pos.z < hc - b*par.s2)
		{	diff = b + pos.z - hc;	if (diff > EPS)  {	norm = make_float3( 0, 0,-1);  addB();  }	}
		
		//  wrap out-in
		diff = pos.y - wmax.y + par.rDexit;
		if (diff > EPS && vel.y > par.rVexit)
		{
			float aa, rr;	// mapping rect-circle
			if (ang < 0.5f)
			{	float xx = xs*2, zz = abs(hc - wmin.z);
				rr = (pos.x + xx/2)/xx * 0.7f;
				aa = (pos.z - zz/2)/zz * 1.6f;
			} else {
				float xx = xs, zz = abs(hc - wmin.z);
				rr = (wmax.x - pos.x)/xs * 0.45f;
				aa = (pos.z - zz/2)/zz * 1.8f;
			}
			rr *= rin;  aa *= PI2;
			float x = cosf(aa) * rr, y = sinf(aa) * rr,
				  z = wmax.z -b - /**/abs(vel.y - par.rVexit)*0.01;
			pos = make_float3(x, y, z);
			vel = make_float3(vel.x,vel.z,-vel.y);
		}
	}	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
}

///----------------------------------------------------------------------------------------------------------------------------
///  Integrate
///----------------------------------------------------------------------------------------------------------------------------

__global__ void integrateD(float4* newPos, float4* newVel, float4* oldPos, float4* oldVel)
{
	int index = __mul24(blockIdx.x,blockDim.x) + threadIdx.x;

	float4 pos4 = oldPos[index];	float4 vel4 = oldVel[index];
	float3 pos = make_float3(pos4);	float3 vel = make_float3(vel4);


	boundary(pos, vel);  // bnd
	
	
	///  Euler integration  -------------------------------
	vel += par.gravity * par.timeStep;	// v(t) = a(t) dt
	vel *= par.globalDamping;	// = 1
	pos += vel * par.timeStep;			// p(t+1) = p(t) + v(t) dt

	/*accel += m_Gravity;  accel *= m_DT;
	vel += accel;				// v(t+1) = v(t) + a(t) dt
	vel_eval += accel;	vel_eval *= m_DT/d;
	pos += vel_eval;	vel_eval = vel;
	///  Leapfrog Integration  ----------------------------
	/*vnext = (accel + m_Gravity) * m_DT + vel;		// v(t+1/2) = v(t-1/2) + a(t) dt
	vel_eval = (vel + vnext) * 0.5f;	// v(t+1) = [v(t-1/2) + v(t+1/2)] * 0.5		used to compute forces later
	vel = vnext;	vnext *= m_DT/d;
	pos += vnext;			// p(t+1) = p(t) + v(t+1/2) dt
	/**/
	
	//----------------  Hard boundary
	float b = par.distBndHard;
	float3 wmin = par.worldMin, wmax = par.worldMax;
	if (pos.x > wmax.x - b)   pos.x = wmax.x - b;
	if (pos.x < wmin.x + b)   pos.x = wmin.x + b;
	if (pos.y > wmax.y - b)   pos.y = wmax.y - b;
	if (pos.y < wmin.y + b)   pos.y = wmin.y + b;
	if (pos.z > wmax.z - b)   pos.z = wmax.z - b;
	if (pos.z < wmin.z + b)   pos.z = wmin.z + b;

	// store new position and velocity
	newPos[index] = make_float4(pos, pos4.w);
	newVel[index] = make_float4(vel, vel4.w);
}



#include "Kernel_Cell.cui"



//----------------------------------------------------------------------------------------------------------------------------
///  Compute SPH  Force
//----------------------------------------------------------------------------------------------------------------------------

__device__ float W(float x, float h){
	if (x<=h){
	return (1-x)/h;
	}
	else return 0.0;
}

__device__ float phiClamp(float I,float Tmin,float Tmax){
return (min(I,Tmax)-min(I,Tmin))/(Tmax-Tmin);
}

__global__ void computeForceD(float4* newPos, float4* newVel, float4* oldPos, float4* oldVel, 
		float4* clr, float* pressure, float* density, float* dyeColor/**/,	uint2* particleHash,  uint* cellStart)
{
	int index = __mul24(blockIdx.x,blockDim.x) + threadIdx.x;

	float4 pos = FETCH(oldPos, index);		float4 vel = FETCH(oldVel, index);
	float pres = FETCH(pressure, index);	float dens = FETCH(density, index);
	int3 gridPos = calcGridPos(pos);

	float3 addVel = make_float3(0.0f);

	//  SPH force, F
	const int s = 1;
	for(int z=-s; z<=s; z++)
	for(int y=-s; y<=s; y++)
	for(int x=-s; x<=s; x++)
		addVel += compForceCell(gridPos + make_int3(x, y, z), index, pos, vel, oldPos, oldVel,
			pres, dens, pressure, density,	particleHash, cellStart);

	volatile uint si = particleHash[index].y;
	
	//  v = F*m*dt    a = F*m   v = a*dt
	addVel *= par.particleMass * par.timeStep;

	
	///  Height map  * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
	if (par.iHmap > 0)
	{	float3 vel3 = make_float3(vel) * -1;
		float rr = par.particleR + par.rotR;
		const int hz = 2,hy = 1;

		if (par.iHmap == 1)  // on XZ
		{		
			int ix = (pos.x -par.worldMin.x)/par.rotSpc;	float xf = ix*par.rotSpc +par.worldMin.x;
			int iz = (pos.z -par.worldMin.z)/par.rotSpc;	float zf = iz*par.rotSpc +par.worldMin.z;
		
			for(int j = -hz; j <= hz; j++)
			for(int i = -hz; i <= hz; i++)
			{
				float xh = xf + i*par.rotSpc;
				float zh = zf + j*par.rotSpc;
				for(int k = 0; k <= hy; k++)
				{
					float xn = xh / par.worldSizeD.x *PI2;
					float zn = zh / par.worldSizeD.z *PI2;
					
					float ss =	sinf(par.s1 * xn + par.s2*PI/180.f) *
								sinf(par.s3 * zn + par.s4*PI/180.f);
					if (par.s5 <= -1.f || ss > par.s5)
					{
						float yf = ss * par.hClose + par.r2Angle + par.worldMin.y;
						float yh = yf - k*par.rotSpc;
						addVel += collideSpheresR( make_float3(xh-pos.x, yh-pos.y, zh-pos.z), vel3, rr);
					}
				}
			}
		} else {	// on YZ  exactly as above, x <-> y
			int iy = (pos.y -par.worldMin.y)/par.rotSpc;	float yf = iy*par.rotSpc +par.worldMin.y;
			int iz = (pos.z -par.worldMin.z)/par.rotSpc;	float zf = iz*par.rotSpc +par.worldMin.z;
		
			for(int j = -hz; j <= hz; j++)
			for(int i = -hz; i <= hz; i++)
			{
				float yh = yf + i*par.rotSpc;
				float zh = zf + j*par.rotSpc;
				for(int k = 0; k <= hy; k++)
				{
					float yn = yh / par.worldSizeD.y *PI2;
					float zn = zh / par.worldSizeD.z *PI2;
					
					float ss =	sinf(par.s1 * yn + par.s2*PI/180.f) *
								sinf(par.s3 * zn + par.s4*PI/180.f);
					if (par.s5 <= -1.f || ss > par.s5)
					{
						float xf = ss * par.hClose + par.r2Angle + par.worldMin.x;
						float xh = xf - k*par.rotSpc;
						addVel += collideSpheresR( make_float3(xh-pos.x, yh-pos.y, zh-pos.z), vel3, rr);
					}
				}
			}
		}
	}

	///  Rotor  ///////////////////////////////////////////////////////////////
	if (par.rotType > 0)
	{	int sx = par.rotSize.x, sy = par.rotSize.y, sz = par.rotSize.z, cb = par.rotBlades;
		float r = par.rotR, sp = par.rotSpc, ca = PI2/cb, x2 = sx*0.5f, y2 = sy*0.5f;
		float rr = par.particleR + r;	float3 vel3 = make_float3(vel) * -1;
		
		switch (par.rotType)
		{
		case 1:		// pump rotor Z -
			if (pos.z > par.collPos.z -rr &&
				pos.z < par.collPos.z +rr + sp*sy)
			for(int z= 1; z<=sz; z++)
			for(int x= 0; x<=sx; x++)	for(int c= 0; c<cb; c++)
			{
				float a = -par.rAngle + c*ca+ z*par.rTwist, cs = cosf(a)*sp, sn = -sinf(a)*sp;
				for(int y= 0; y<=sy; y++)	//..if bnd xy
				{
					float4 pc = make_float4((x-x2)*cs - z*sn, (x-x2)*sn + z*cs, sp*y, 0);
						addVel += collideSpheresR( make_float3(par.collPos + pc - pos), vel3, rr);
				}
			}	break;

		case 2:		// pump rotor Y |
			if (pos.y > par.collPos.y -rr &&
				pos.y < par.collPos.y +rr + sp*sy)
			for(int z= 1; z<=sz; z++)
			for(int x= 0; x<=sx; x++)	for(int c= 0; c<cb; c++)
			{
				float a = -par.rAngle + c*ca+ z*par.rTwist, cs = cosf(a)*sp, sn = -sinf(a)*sp;
				for(int y= 0; y<=sy; y++)
				{
					float4 pc = make_float4((x-x2)*cs - z*sn, sp*y, (x-x2)*sn + z*cs, 0);
						addVel += collideSpheresR( make_float3(par.collPos + pc - pos), vel3, rr);
				}
			}	break;

		case 3:		// propeller Z -
			if (pos.z > par.collPos.z -rr - sp*sz/2.f &&
				pos.z < par.collPos.z +rr + sp*sz/2.f)
			{
			float4 rotPos = par.collPos;
			float aa = par.rAngle, tw = par.rTwist;
			if (par.r2Dist > 0.f)
				if (pos.x > 0.f) {  rotPos.x += par.r2Dist*0.5f;  }
				else			 {  rotPos.x -= par.r2Dist*0.5f;  aa = par.r2Angle;  tw *= par.r2twist;  }
				
			for(int c= 0; c<cb; c++)	{	float a = aa + c*ca;	// h = d-  x = r|
			for(int h= 0; h<=sy; h++)
			{	float dh = 0;  if (h==sy-1) dh=0.5f; else if (h==sy)  dh=1.2f;
				for(int x= 0; x<=sz; x++)
				{		float d = dh;  if (x==sz && d==0) d=0.4f;
						float k = cosf( (x-x2)*0.2f + PI*0.6f);  //d modif -^.
						float at = tw*(d*-0.05f -(h-y2)*k);	
					float ac = a + at, cs = cosf(ac)*sp, sn = -sinf(ac)*sp;
					float4 pc = make_float4(x*cs, x*sn, sp*(-d+h-y2) *k, 0);
					r = par.rotR*abs(1-d);
					addVel += collideSpheresR( make_float3(rotPos + pc - pos), vel3, par.particleR + r);
				}
			}	}
			}	break;
		}
	}
	///  sphere collider
	else
		addVel += collideSpheres(par.collPos - pos, -vel, par.particleR + par.collR);


	///  Accelerators  *************************************
	for (int i=0; i < NumAcc; i++)
	{	Accel& ac = par.acc[i];
		if (ac.type != ACC_Off)
		{
			float3 rel = make_float3(pos) - ac.pos;
			switch (ac.type)
			{
				case ACC_Box:	if (abs(rel.x) < ac.size.x && 
						abs(rel.y) < ac.size.y && abs(rel.z) < ac.size.z)
						addVel += ac.acc * par.timeStep;	break;
			
				case ACC_CylY:	{	float r = length(make_float2(rel.x/ac.size.x, rel.z/ac.size.z));
					if (abs(rel.y) < ac.size.y && r < 1.f)
						//rel.x*rel.x < ac.size.x*ac.size.x && rel.z*rel.z < ac.size.z*ac.size.z)
						addVel += ac.acc * par.timeStep;	}	break;
			
				case ACC_CylYsm: {	float r = length(make_float2(rel.x, rel.z));
					if (abs(rel.y) < ac.size.y && r < ac.size.x)
						addVel += ac.acc * (1.f - r/ac.size.z) * par.timeStep;  }  break;
		}	}
	}

	//  add new vel
	newVel[si] = vel + make_float4(addVel, 0.0f);


	///  coloring  . . . . . . . . . . . . . . . . . . . . . . . . . . . . 
	float3 color = make_float3(0.2, 0.5, 1);  float intens = 0.f;
	
	switch (par.clrType)
	{	case CLR_None:  return;
		
		case CLR_VelAcc:
		{	float v = 2.5f * length(make_float3(vel));
			float f = 0.02f * length(addVel) / par.timeStep;
				float clrV = par.brightness + par.contrast * v;
				float clrF = par.contrast * f;
			color = color * clrV + make_float3(0.7f, 0.35f, 0) * clrF;	}	break;

		case CLR_DensAcc:
		{	float d = 4.f* (dens - par.restDensity) / par.restDensity;
			float f = 0.02f * length(addVel) / par.timeStep;
				float clrD = par.brightness + par.contrast * d;
				float clrF = par.contrast * f;
			color = color * clrD + make_float3(0.7f, 0.7f, 0) * clrF;	}	break;
		
		case CLR_Vel:
		{	float v = 2.5f * length(make_float3(vel));
				float clrV = par.brightness + par.contrast * v;		intens = clrV;

	//* Foam implementation
    float4 NvelDif = make_float4(0.0f); 
	float4 NposDif = make_float4(0.0f); 
	float minvThreshold = 5.0f;
	float maxvThreshold = 20.0f;
	float vdiff = 0.0f; 
	float TrappedAirPot = 0.0f;
	uint gridHash = calcGridHash(gridPos);
	uint bucketStart = FETCH(cellStart, gridHash);
	float h = 3.0f;
    float3 Normal = make_float3(0);
	 
	 for(int x=-1;x<2;x++){
	 for(int y=-1;y<2;y++){
	 for(int z=-1;z<2;z++){
	//  iterate over particles in this cell
	for (uint i=0; i < par.maxParInCell; i++)
	{
	 	uint gridHash = calcGridHash(make_int3(gridPos.x+x,gridPos.y+y,gridPos.z+z));
    	uint bucketStart = FETCH(cellStart, gridHash);   
		uint index2 = bucketStart + i;
		uint2 cellData = FETCH(particleHash, index2);
		if (cellData.x != gridHash)  break;

		if (index2 != index)
		{
			float4 vel2 = FETCH(oldVel, index2); 
			float4 pos2 = FETCH(oldPos, index2);
			NvelDif = normalize(vel+vel2); //nvlog error when '-' ??
			NposDif = normalize(pos+pos2); //nvlog error when '-' ??
			vdiff += sqrt(dot(vel-vel2,vel-vel2)) * (1.0f-dot(NvelDif,NposDif)) * W(sqrt(dot(pos-pos2,pos-pos2)),h);

			/*
			Normal.x += (pos.y-pos2.y)*(pos.z-pos2.z);
			Normal.y += (pos.x-pos2.x)*(pos.z-pos2.z);
			Normal.z += (pos.y-pos2.y)*(pos.x-pos2.x);
			*/
		}
	}	
	}}}

	TrappedAirPot = phiClamp(vdiff,minvThreshold,maxvThreshold);

	/*
	float3 Normal;
	float4 pos2 = FETCH(oldPos, index+1); 
	float4 pos3 = FETCH(oldPos, index-1);
	float4 U = pos2-pos;
	float4 V = pos3-pos;

	Normal.x = U.y*V.z - U.z*V.y;
	Normal.y = U.z*V.x - U.x-V.z;
	Normal.z = U.x*V.y - U.y*V.x;
	*/
	float Iwc = 0;

	float minkThreshold = 0.1f;
	float maxkThreshold = 1.0f;
	float kEnergy = par.particleMass*1000*dot(vel,vel)*0.5;
	float kPot = phiClamp(kEnergy,minkThreshold,maxkThreshold);

	float Kta = 3.0f;
	float Kwc = 3.0f;
	float Nd = kPot*(Kta*TrappedAirPot+Kwc*Iwc);

	//color *= clrV;

    color = make_float3(Nd); 
	
	}	break;

//*End of modifications
		case CLR_VelRGB:
		{	color = make_float3(0.5f, 0.5f, 0.5f) + par.contrast * 2.5f * make_float3(vel);	}	break;

		case CLR_Accel:
		//{	float f = 0.000005f / par.particleMass * length(addVel) / par.timeStep;  // =force-  v = F*m*dt
		{	float f = 0.02f * length(addVel) / par.timeStep;	//  = accel   v = a*dt
				float clrF = par.brightness + par.contrast * f;		intens = clrF;
			color *= clrF;  }	break;

		case CLR_Dens:
		{	float d = 4.f* (dens - par.restDensity) / par.restDensity;
				float clrD = par.brightness + par.contrast * d;		intens = clrD;
			color *= clrD;	}	break;
	}
	if (par.iHue == 1)	{  color.x = intens;/*Hue*/  color.y = 0.f;  }

	
	///  dye  ` ` ` ` ` `
	if (par.dyeClear > 0)  dyeColor[si] = 0.f;
	else if (par.dyeType > 0)
	{
		float dyeCl = FETCH(dyeColor, si);  // intensity

		//  set clr
		float3 rel = make_float3(pos) - par.dyePos;
		switch (par.dyeType)
		{
		case 1:  // box
		{	float3& dsize = par.dyeSize;
			if (abs(rel.x) < dsize.x && abs(rel.y) < dsize.y && abs(rel.z) < dsize.z)	dyeCl = 1.f;  }  break;
		case 2:  // sphere
			if (length(rel) < par.dyeSize.y)	dyeCl = 1.f;  break;
		}

		//  fade
		dyeCl -= par.timeStep * par.dyeFade;
		if (dyeCl < 0.f)  dyeCl = 0.f;//
		if (dyeCl >= 0.f)
			dyeColor[si] = dyeCl;

		if (par.iHue == 0)
			color += make_float3(0.9f,0.9f,1)* dyeCl;
		else
			color.y = dyeCl;
	}
	clr[si] = make_float4(color, 1.f);
}
// END OF KERNEL.CU!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

extern "C"
{

//  Utility, data
#define CU  CUDA_SAFE_CALL

#define cuMapVbo(pv,vbo)		CU(cudaGLMapBufferObject((void**)&pv, vbo));
#define cuUnMapVbo(vbo)			CU(cudaGLUnmapBufferObject(vbo));

#define cuBindTex(tex,pv,size)	CU(cudaBindTexture(0, tex, pv, size));
#define cuUnbindTex(tex)		CU(cudaUnbindTexture(tex));


//void cudaInit(int argc, char **argv)	{   CUT_DEVICE_INIT(argc, argv);	}
/*bool cudaInitB(int argc, char **argv, bool showInfo)	//  no exit + info
{
	int deviceCount;
	CUDA_SAFE_CALL_NO_SYNC(cudaGetDeviceCount(&deviceCount));
	if (deviceCount == 0)  {
		fprintf(stderr, "Error: no devices supporting CUDA.\n");
		return false;  }  //exit(EXIT_FAILURE);
	int dev = 0;
	cutGetCmdLineArgumenti(argc, (const char**)argv, "device", &dev);
	if (dev > deviceCount-1) dev = deviceCount - 1;
	cudaDeviceProp p;
	CUDA_SAFE_CALL_NO_SYNC(cudaGetDeviceProperties(&p, dev));
	if (p.major < 1)  {
		fprintf(stderr, "Error: device does not support CUDA.\n");
		return false;	}
    if (dev == 0)  {
        if (p.major == 9999 && p.minor == 9999)
            printf("There is no device supporting CUDA.\n");
        else  if (deviceCount != 1)
            printf("There are %d devices supporting CUDA\n", deviceCount);  }
	
	if (cutCheckCmdLineFlag(argc, (const char**)argv, "quiet") == CUTFalse)
		fprintf(stderr, "Using device %d: %s\n\n", dev, p.name);
		
	if (showInfo)
	{
        printf("CUDA version:  %d.%d\n", p.major, p.minor);
        printf("Total memory:  %u bytes  (%u MB)\n", p.totalGlobalMem, p.totalGlobalMem/1024/1024);
		#if CUDART_VERSION >= 2000
        printf("Multiprocessors:  %d  Cores:  %d\n", p.multiProcessorCount, 8*p.multiProcessorCount);
		#endif
		printf("Clock rate:  %.4f GHz\n", p.clockRate * 1e-6f);
		printf(" Warp size:  %d\n", p.warpSize);
		printf(" Constant memory:  %u bytes\n", p.totalConstMem); 
		printf("  Shared memory per block:  %u bytes\n", p.sharedMemPerBlock);
		printf("  Registers per block:   %d\n", p.regsPerBlock);
		printf("  Max threads per block: %d\n", p.maxThreadsPerBlock);
		printf("  Max sizes of a block:  %d x %d x %d\n", p.maxThreadsDim[0], p.maxThreadsDim[1], p.maxThreadsDim[2]);
		printf("  Max sizes of a grid:   %d x %d x %d\n", p.maxGridSize[0], p.maxGridSize[1], p.maxGridSize[2]);
		printf("  Max memory pitch:    %u bytes\n", p.memPitch);
		printf("  Texture alignment:   %u bytes\n", p.textureAlignment);
	    #if CUDART_VERSION >= 2000
        printf("  Concurrent copy and execution:  %s\n\n", p.deviceOverlap ? "Yes" : "No");
		#endif
	}
	
	CUDA_SAFE_CALL(cudaSetDevice(dev));
	return true;
}*/
void threadSync()						{	CU(cudaThreadSynchronize());	}


void allocateArray(void **devPtr, size_t size)	{	CU(cudaMalloc(devPtr, size));	}
void freeArray(void *devPtr)					{	CU(cudaFree(devPtr));	}

void registerGLvbo(uint vbo)		{	CU(cudaGLRegisterBufferObject(vbo));	}
void unregGLvbo(uint vbo)			{	CU(cudaGLUnregisterBufferObject(vbo));	}


void copyFromDevice(void* host, const void* device, unsigned int vbo, int size)
{
	if (vbo)  cuMapVbo(device, vbo);
	CU(cudaMemcpy(host, device, size, cudaMemcpyDeviceToHost));
	if (vbo)  cuUnMapVbo(vbo);
}
void copyToDevice(void* device, const void* host, int offset, int size)
{
	CU(cudaMemcpy((char*) device + offset, host, size, cudaMemcpyHostToDevice));
}

void setParameters(SimParams *hostParams)
{	// copy parameters to constant memory
	CU(cudaMemcpyToSymbol(par, hostParams, sizeof(SimParams)));
}


//  Round a / b to nearest higher integer value
int iDivUp(int a, int b) {	return a%b != 0 ? a/b+1 : a/b;	}

//  compute grid and thread block size for a given number of elements
void computeGridSize(int n, int blockSize, int &numBlocks, int &numThreads)
{
	numThreads = min(blockSize, n);  numBlocks = iDivUp(n, numThreads);
}
//----------------------------------------------------------------------------------



///  integrate
void integrate(uint vboOldPos, uint vboNewPos, float4* oldVel, float4* newVel, int numParticles)
{
	int numThreads, numBlocks;
	computeGridSize(numParticles, 256, numBlocks, numThreads);

	float4 *oldPos, *newPos;
	cuMapVbo(oldPos, vboOldPos);  cuMapVbo(newPos, vboNewPos);

	integrateD<<< numBlocks, numThreads >>>(newPos, newVel, oldPos, oldVel);

	CUT_CHECK_ERROR("Kernel execution failed: Integrate");
	cuUnMapVbo(vboOldPos);  cuUnMapVbo(vboNewPos);

	/***/threadSync();
}


///  calcHash
void calcHash(uint vboPos, uint2* particleHash, int numParticles)
{
	int numThreads, numBlocks;
	computeGridSize(numParticles, 2*256, numBlocks, numThreads);

	float4 *pos;
	cuMapVbo(pos, vboPos);

	calcHashD<<< numBlocks, numThreads >>>(pos, particleHash);

	CUT_CHECK_ERROR("Kernel execution failed: calcHash");
	cuUnMapVbo(vboPos);

	/***/threadSync();
}


///  reorder
void reorder(uint vboOldPos, float4* oldVel, float4* sortedPos, float4* sortedVel,
		uint2* particleHash, uint* cellStart,  uint numParticles, uint numCells)
{
	int numThreads, numBlocks;
	computeGridSize(numParticles, 256, numBlocks, numThreads);
	CU(cudaMemset(cellStart, 0xffffffff, numCells*sizeof(uint)));

	float4 *oldPos;
	cuMapVbo(oldPos, vboOldPos);
	#if USE_TEX
	uint spar4 = numParticles*sizeof(float4);
	cuBindTex(oldPosTex, oldPos, spar4);	cuBindTex(oldVelTex, oldVel, spar4);
	#endif

	reorderD<<< numBlocks, numThreads >>>(particleHash, cellStart,
		oldPos, oldVel, sortedPos, sortedVel);

	CUT_CHECK_ERROR("Kernel execution failed: reorder");
	#if USE_TEX
	cuUnbindTex(oldPosTex);  cuUnbindTex(oldVelTex);
	#endif
	cuUnMapVbo(vboOldPos);

	/***/threadSync();
}


///  collide
void collide(uint timer,  uint vboOldPos, uint vboNewPos, /**/uint vboCLR2,
		float4* sortedPos, float4* sortedVel, float4* oldVel, float4* newVel,
		float* pressure, float* density, float* dyeColor,
		uint2* particleHash, uint* cellStart, uint numParticles, uint numCells)
{
//	/**/CU(cutStartTimer(timer));
	float4 *oldPos, *newPos, *clr2;
	cuMapVbo(clr2, vboCLR2);
	cuMapVbo(oldPos, vboOldPos);  cuMapVbo(newPos, vboNewPos);  

	#if USE_TEX
	uint spar4 = numParticles*sizeof(float4), spar = numParticles*sizeof(float);
	cuBindTex(oldPosTex, sortedPos, spar4);  cuBindTex(pressureTex, pressure, spar);
	cuBindTex(oldVelTex, sortedVel, spar4);	 cuBindTex(densityTex, density, spar);  cuBindTex(dyeColorTex, dyeColor, spar);//
	cuBindTex(particleHashTex, particleHash, numParticles*sizeof(uint2));
	cuBindTex(cellStartTex, cellStart, numCells*sizeof(uint));
	#endif


	int numThreads, numBlocks;
	computeGridSize(numParticles, 64, numBlocks, numThreads);
	

	computeDensityD<<< numBlocks, numThreads >>>(clr2, sortedPos,  pressure, density,  particleHash, cellStart);
	/***/threadSync();
//	/**/CU(cutStopTimer(timer));

	  computeForceD<<< numBlocks, numThreads >>>(newPos, newVel, sortedPos, sortedVel,  clr2, pressure, density, dyeColor/**/, particleHash, cellStart);
		//collideD<<< numBlocks, numThreads >>>(newPos, newVel, sortedPos, sortedVel, /**/clr2,  particleHash, cellStart);

	CUT_CHECK_ERROR("Kernel execution failed: Collide");

	cuUnMapVbo(vboNewPos);  cuUnMapVbo(vboOldPos);  cuUnMapVbo(vboCLR2);
	#if USE_TEX
	cuUnbindTex(oldPosTex);		cuUnbindTex(oldVelTex);
	cuUnbindTex(pressureTex)	cuUnbindTex(densityTex);	cuUnbindTex(dyeColorTex);//
	cuUnbindTex(particleHashTex);	cuUnbindTex(cellStartTex);
	#endif
	/***/threadSync();
}


}  //extern "C"