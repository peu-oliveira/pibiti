#version 330 core
#extension GL_NV_shadow_samplers_cube : enable

	
		uniform float SCR_HEIGHT;
		uniform float SCR_WIDTH;
		uniform float camPosx;
		uniform float camPosy;
		uniform float camPosz;
		uniform mat4 Projection;
		uniform mat4 ModelView;

		layout(location = 0) out vec4 FragColor;
		in vec2 TexCoords;
		uniform sampler2D depth;
		uniform sampler2D gPosition;
		uniform sampler2D particleThickness;
		uniform samplerCube skybox;
		uniform sampler2D FoamDepthTexture;
        uniform sampler2D FoamDepthStencilTexture;
		vec3 currNorm = vec3(0.0);

		float dzdx(int n) {
			if (TexCoords.y >= 1 || TexCoords.x >= 1 || TexCoords.y < 0 || TexCoords.x < 0)
				return 0;
			vec2 x0 = vec2(TexCoords.x + (1.0 / SCR_WIDTH) + (1.0 / SCR_WIDTH) * n, TexCoords.y);
			vec2 x1 = vec2(TexCoords.x - (1.0 / SCR_WIDTH) + (1.0 / SCR_WIDTH) * n, TexCoords.y);
			float z0 = texture(depth, x0).r;
			float z1 = texture(depth, x1).r;
			return (z0 - z1) / 2.0;
		}

		//	dz/dy
		float dzdy(int n) {
			if (TexCoords.y >= 1 || TexCoords.x >= 1 || TexCoords.y < 0 || TexCoords.x < 0)
				return 0;
			vec2 y0 = vec2(TexCoords.x, TexCoords.y + (1.0 / SCR_HEIGHT) + (1.0 / SCR_HEIGHT) * n);
			vec2 y1 = vec2(TexCoords.x, TexCoords.y - (1.0 / SCR_HEIGHT) + (1.0 / SCR_HEIGHT) * n);
			float z0 = texture(depth, y0).r;
			float z1 = texture(depth, y1).r;
			return (z0 - z1) / 2.0;
		} 
        void getNormals(float Z) {

			float height = SCR_HEIGHT;
			float width = SCR_WIDTH;
			float j = TexCoords.y;
			float i = TexCoords.x;

			if (Z > 0)
			{
				float dz_x = dzdx(0);
				float dz_x0 = dzdx(-1);
				float dz_x2 = dzdx(1);
				float dz2x2 = (dz_x2 - dz_x0) / 2.0;

				float dz_y = dzdy(0);
				float dz_y0 = dzdy(-1);
				float dz_y2 = dzdy(1);
				float dz2y2 = (dz_y2 - dz_y0) / 2.0;

				float Cx = i == 0 ? 0 : 2.f / (width * i);
				float Cy = j == 0 ? 0 : 2.f / (height * j);
				float D = Cy * Cy * dz_x * dz_x + Cx * Cx * dz_y * dz_y + Cx * Cx * Cy * Cy * Z * Z;

				if (D != 0)
				{
					float rv_sqrtD = 1.f / sqrt(D);
					currNorm.x = (-Cy * dz_x * rv_sqrtD);
					currNorm.y = (-Cx * dz_y * rv_sqrtD);
					currNorm.z = (Cx * Cy * (1 - Z) * rv_sqrtD);
				}}
		}

float fresnel ( float rr1 , float rr2 , vec3 n , vec3 d )
{
float r = rr1 / rr2 ;
float theta1 = dot( n , -d ) ;
float theta2 = sqrt( 1.0 - r * r * ( 1.0 - theta1 * theta1 ) ) ;
// Fi g u r e out what the F r e s n el equations say about what happens next
float rs = ( rr1 * theta1 - rr2 * theta2 ) / ( rr1 * theta1 + rr2 * theta2 ) ;
rs = rs * rs ;
float rp = ( rr1 * theta2 - rr2 * theta1 ) / ( rr1 * theta2 + rr2 * theta1 ) ;
rp = rp * rp ;
return( ( rs + rp ) / 2.0 ) ;
}


		void main() {
		    float gamma = 2.2;
			vec3 fPos = texture(gPosition, TexCoords).rgb; //Actually cubemap scene tex
			vec4 FoamDepthStencil = texture(FoamDepthStencilTexture,TexCoords);
			float FoamDepth = texture(FoamDepthTexture,TexCoords).r;
			float thickness = texture(particleThickness, TexCoords).x*0.075;
			float Z = texture(depth, TexCoords).r;
			if (Z <= 0.0 || Z==1)
				discard;
			getNormals(Z);
			currNorm = normalize(currNorm);

			const vec3 lightDir = vec3(0.0,-10.0,-1.0);
			vec3 LightIntensity = pow(vec3(0.252,0.212,0.15),vec3(gamma));

			float mag = dot(currNorm.xy, currNorm.xy);
			if (mag > 1.0)
				discard; // don't draw outside circle

			vec2 pos = (TexCoords-vec2(0.5))*2.0;
			vec3 eyePos = (Z*vec3(-pos.x*Projection[0][0],-pos.y*Projection[1][1],1.0));
			eyePos = (vec4(eyePos,1.0)*inverse(ModelView)).xyz;
            vec3 camPos = vec3(camPosx,camPosy,camPosz);
			vec3 fromEye = normalize(eyePos);
			vec4 background = texture(gPosition,TexCoords);
			float Far = 1.0, Near = 0.1;
			float backgroundDepth = ( 2.0 * Near ) / ( Far + Near - ( 1.0/*backgroundDepthTexture*/) * ( Far - Near ));


			// calculate lighting
			vec4 waterColor = vec4(0.2, 0.5, 0.7, 1.0);
			waterColor.xyz = pow(waterColor.xyz,vec3(gamma));
			vec3 reflectedEye = normalize(reflect(fromEye,currNorm));
			vec4 environmentColor = textureCube(skybox,reflectedEye);
			vec4 sceneCol = texture(gPosition,TexCoords + (currNorm.xy*thickness));

			float lambert = max( 0.0 , dot( normalize( lightDir.xyz ) , currNorm ) ) ;
            float cosine = max( 0.0 , dot( normalize ( lightDir.xyz - eyePos ) , currNorm ) ) ;
            vec4 ambient = waterColor;
            vec4 diffuse = waterColor * cosine;
            float Fspecular = clamp(fresnel( 1.0 , 1.33 , currNorm , fromEye ) , 0.0 ,1.0) ; //To check values
            vec4 absorbColor = ( vec4( LightIntensity , 1.0 ) * ( ambient + diffuse ) ) * exp(-thickness ) ;
            vec4 foamColor = vec4(1.0);
			if(FoamDepthStencil.w==1.0) discard;
if ( ( Z == 0.0 || Z==1.0 /*|| ( Z != 0.0 && particleDepthStencil.r != 0.0 )*/ )&& ( FoamDepth == 0.0 || ( FoamDepth != 0.0 && FoamDepthStencil.r != 0.0 ) ) ) //Still need stencil values
{
FragColor = background ;
gl_FragDepth = backgroundDepth ;
}
else{
            FragColor = mix(mix( ( lambert + 0.2 ) * absorbColor * ( 1.0 - Fspecular ) + (Fspecular) * environmentColor/2.0 , foamColor , FoamDepthStencil.w/*0*/ ) , sceneCol/2.0 , 0.5 );///2.0; //Division by 2 because of gamma correction
			FragColor.xyz = pow(FragColor.xyz,vec3(1/gamma));
}
		//	FragColor = vec4(FoamDepthStencil.x);
		}