#version 330 core
	
		uniform float SCR_HEIGHT;
		uniform float SCR_WIDTH;
		uniform float camPosx;
		uniform float camPosy;
		uniform float camPosz;

		layout(location = 0) out vec4 FragColor;
		in vec2 TexCoords;
		uniform sampler2D depth;
		uniform sampler2D gNormal;
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

		void main() {
			vec3 fNormal = texture(gNormal, TexCoords).rgb;
			float Z = texture(depth, TexCoords).r;
			if (Z <= 0.0 || Z==1)
				discard;
			getNormals(Z);


			const vec3 lightDir = vec3(0.577, 0.577, 0.577);

			float mag = dot(currNorm.xy, currNorm.xy);
			if (mag > 1.0)
				discard; // don't draw outside circle

            vec3 camPos = vec3(camPosx,camPosy,camPosz);

			// calculate lighting
			vec3 I = normalize(fNormal-camPos);
			float diffuse = max(0.0, dot(lightDir, currNorm));
			float ratio = 1.00 / 1.33;
			vec3 R = refract(I, normalize(currNorm), ratio);
			vec3 R2 = reflect(I, normalize(currNorm));

			FragColor = (vec4(texture(skybox, R).rgb, 1.0) * 0.4 + vec4(0.2, 0.5, 1.0, 1.0) * 0.2 + vec4(texture(skybox, R2).rgb, 1.0) * 0.4);
			//FragColor = vec4(vec3(Z),1.0);
			//FragColor = vec4(fNormal, 1.0);
		}