#version 330 core
	
		uniform float fAmbient; // factors
		uniform float fDiffuse;
		uniform float fPower;
		uniform float SCR_HEIGHT;
		uniform float SCR_WIDTH;

		layout(location = 0) out vec4 FragColor;
		in vec2 TexCoords;
		uniform sampler2D depth;
		uniform sampler2D gPosition;
		uniform sampler2D gNormal;
		//uniform sampler2D gAlbedoSpec;
		uniform samplerCube skybox;
		vec3 currNorm = vec3(0.0);

		float dzdx(int n) {
			if (TexCoords.y >= 1 || TexCoords.x >= 1 || TexCoords.y < 0 || TexCoords.x < 0)
				return 0;
			vec2 x0 = vec2(TexCoords.x + (1.0 / SCR_WIDTH) + (1.0 / SCR_WIDTH) * n, TexCoords.y);
			vec2 x1 = vec2(TexCoords.x - (1.0 / SCR_WIDTH) + (1.0 / SCR_WIDTH) * n, TexCoords.y);
			float z0 = texture(depth, x0).r;
			float z1 = texture(depth, x1).r;
			//if (abs(z0 - z1) > 0.01) return 0;
			if (z0 == 1.0 || z1 == 1.0)
				return 0;
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
			//if (abs(z0 - z1) > 0.01) return 0;
			if (z0 == 1.0 || z1 == 1.0)
				return 0;
			return (z0 - z1) / 2.0;
		} 
                        float curvature_flow_step(float Z) {
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
				float inv_D32 = 1.f / pow(D, 1.5);

				float ky = 4.f / height / height;
				float kx = 4.f / width / width;
				float dD_x = ky * pow(j, -2.f) * 2 * dz_x * dz2x2 +
							 kx * dz_y * dz_y * -2 * pow(i, -3.f) +
							 ky * pow(j, -2.f) * kx * (-2 * pow(i, -3.f) * Z * Z + pow(i, -2.f) * 2 * Z * dz_x);

				float dD_y = kx * pow(i, -2.f) * 2 * dz_y * dz2y2 +
							 ky * dz_x * dz_x * -2 * pow(j, -3.f) +
							 kx * pow(i, -2.f) * ky * (-2 * pow(j, -3.f) * Z * Z + pow(j, -2.f) * 2 * Z * dz_y);

				float Ex = 0.5 * dz_x * dD_x - dz2x2 * D;
				float Ey = 0.5 * dz_y * dD_y - dz2y2 * D;
				if (D != 0)
				{
					float rv_sqrtD = 1.f / sqrt(D);
					currNorm.x = (-Cy * dz_x * rv_sqrtD);
					currNorm.y = (-Cx * dz_y * rv_sqrtD);
					currNorm.z = (Cx * Cy * (1 - Z) * rv_sqrtD);
				}
				//else currNorm = vec3(0);
				return (Cy * Ex + Cx * Ey) * inv_D32 / 2.0;
			}
			else
				return 0;
		}

		void main() {
			vec3 fPos = texture(gPosition, TexCoords).rgb;
			vec3 fNormal = texture(gNormal, TexCoords).rgb;
			//	vec4 fAlbedo = texture(gAlbedoSpec, TexCoords).rgba;
			float Z = texture(depth, TexCoords).r;
			if (Z <= 0.0)
				discard;
			float FlowDepth = curvature_flow_step(Z);
			if (FlowDepth > 1.0)
				FlowDepth = 1.0;
			if (FlowDepth < -1.0)
				FlowDepth = -1.0;

			Z = Z - (FlowDepth * 0.001);
			if (Z > 1)
				Z = 1.0;
			if (Z < 0)
				Z = 0.0;

			const vec3 lightDir = vec3(0.577, 0.577, 0.577);

			float mag = dot(currNorm.xy, currNorm.xy);
			if (mag > 1.0)
				discard; // don't draw outside circle
					//currNorm.z = sqrt(1.0 - mag);

			// calculate lighting
			float diffuse = max(0.0, dot(lightDir, currNorm));
			float ratio = 1.00 / 1.33;
			vec3 R = refract(fPos, normalize(currNorm), ratio);
			vec3 R2 = reflect(fPos, normalize(currNorm));

			//  if(FlowDepth>0) FragColor = fAlbedo*(fAmbient + fDiffuse * pow(diffuse, fPower));
			gl_FragDepth = Z;
			FragColor = (vec4(texture(skybox, R).rgb, 1.0) * 0.6 + vec4(0.2, 0.5, 0.7, 1.0) * 0.1 + vec4(texture(skybox, R2).rgb, 1.0) * 0.3);
			//FragColor = vec4(vec3(Z),1.0);
			//FragColor = vec4(currNorm, 1.0);
		}