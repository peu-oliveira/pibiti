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
// Fi g u r e out what the F r e s n el e q u a ti o n s say about what happens next
float rs = ( rr1 * theta1 - rr2 * theta2 ) / ( rr1 * theta1 + rr2 * theta2 ) ;
rs = rs * rs ;
float rp = ( rr1 * theta2 - rr2 * theta1 ) / ( rr1 * theta2 + rr2 * theta1 ) ;
rp = rp * rp ;
return( ( rs + rp ) / 2.0 ) ;
}


		void main() {
			vec3 fPos = texture(gPosition, TexCoords).rgb; //Actually cubemap scene tex
			float thickness = texture(particleThickness, TexCoords).x*0.075;
			float Z = texture(depth, TexCoords).r;
			if (Z <= 0.0 || Z==1)
				discard;
			getNormals(Z);
			currNorm = normalize(currNorm);

			const vec3 lightDir = vec3(0.0,1.0,-1.0);

			float mag = dot(currNorm.xy, currNorm.xy);
			if (mag > 1.0)
				discard; // don't draw outside circle

			vec2 pos = (TexCoords-vec2(0.5))*2.0;
			vec3 eyePos = (Z*vec3(-pos.x*Projection[0][0],-pos.y*Projection[1][1],1.0));
			eyePos = (vec4(eyePos,1.0)*inverse(ModelView)).xyz;
            vec3 camPos = vec3(camPosx,camPosy,camPosz);
			vec3 fromEye = normalize(eyePos);
			// calculate lighting
			vec4 waterColor = vec4(0.2, 0.5, 0.7, 1.0);
			vec3 LightIntensity = vec3(0.5,0.5,0.5);
			vec3 reflectedEye = normalize(reflect(fromEye,currNorm));
			vec4 environmentColor = textureCube(skybox,reflectedEye);
			vec4 sceneCol = texture(gPosition,TexCoords + (currNorm.xy*thickness));

			float lambert = max( 0.0 , dot( normalize( lightDir.xyz ) , currNorm ) ) ;
            float cosine = max( 0.0 , dot( normalize ( lightDir.xyz - eyePos ) , currNorm ) ) ;
            vec4 ambient = waterColor;
            vec4 diffuse = waterColor * cosine;
            float Fspecular = clamp(fresnel( 0.25 , 0.5 , currNorm , fromEye ) , 0.0 ,0.2) ;
            vec4 absorbColor = ( vec4( LightIntensity , 1.0 ) * ( ambient + diffuse ) ) * exp(-thickness ) ;
            vec4 foamColor = vec4(1.0);
            FragColor = mix(mix( ( lambert + 0.2 ) * absorbColor * ( 1.0 - Fspecular ) + (1.0-Fspecular) * environmentColor , foamColor , 0.1/*Actually FoamDepth*/ ) , sceneCol , 0.5 );
			//FragColor = vec4(thickness);
			//** Old rendering
/*
   	   //   FragColor = environmentColor;
			float lightSpecular = 0.25;
			float materialShininess = 0.3;
			vec3 I = normalize(fPos-camPos);
			//float diffuse = max(0.0, dot(lightDir, currNorm));
			float ratio = 1.00 / 1.33;
			vec3 R = refract(I, normalize(currNorm), ratio);
			vec3 R2 = reflect(I, normalize(currNorm));

		   vec3 viewDir = normalize(camPos - fPos);
           vec3 reflectDir = reflect(-lightDir, currNorm);  
           float spec = pow(max(dot(viewDir, reflectDir), 0.0), materialShininess);
           vec3 specular = lightSpecular * spec * texture(skybox, R).rgb;  
*/
		    //FragColor = (vec4(texture(skybox, R).rgb, 0.0) * 0.4 + waterColor * 0.2 + vec4(texture(skybox, R2).rgb, 0.0) * 0.4) + vec4(specular,1.0)*0.5 + diffuse;
			//FragColor = vec4(vec3(Z),1.0);
			//FragColor = vec4(fNormal, 1.0);
			//FragColor = thickness;
		}