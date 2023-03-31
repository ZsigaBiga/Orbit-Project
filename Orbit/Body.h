#pragma once

class Body {
public:
	int id;
	int largestId = 0;
	float mass;
	float radius;
	Vector3 initialVelocity = Vector3Zero();
	Vector3 position;
	Vector3* trailPoints = new Vector3[1000];
	Vector3 forceOut = {0};
	Model body;
	Color bodCol;

	void UpdateVelocity(float timestep, std::vector<Body>& bodies) {
		for (size_t i = 0; i < bodies.size(); i++) {
			if (bodies[i] != *this)
			{
				//F = sqrt(G*M*m1 / r^2) -- Force equals the square root of gravitational constant times body1 mass times body2 mass divided by their distance squared

				float sqrDst = Vector3DistanceSqr(bodies[i].position, position);
				Vector3 forceDir = Vector3Normalize(Vector3Subtract(bodies[i].position, position));
				Vector3 force = Vector3Scale(forceDir, G);
				force = Vector3Add(force, Vector3Scale(force, bodies[i].mass));
				force = Vector3Add(force, Vector3Scale(force, mass));
				force = Vector3Add(force, Vector3Scale(force, (1.0f / sqrDst)));
				forceOut = force;
				Vector3 acceleration = Vector3Scale(force, (1.0f / mass));

				currentVelocity = Vector3Add(currentVelocity, Vector3Scale(acceleration, timestep));

				/*float pitchAng = asinf(-currentVelocity.y);
				float yawAng = atan2f(currentVelocity.x, currentVelocity.z);
				float rollAng = atan2f(currentVelocity.x, currentVelocity.y);
				*/ //Spaceship facing

				//body.transform = MatrixRotateXYZ(Vector3{ 0.0f });
				//std::cout << TextFormat("Velocity Vector: %f, %f, %f", currentVelocity.x, currentVelocity.y, currentVelocity.z);
				//std::cout << TextFormat("Other obj position: %f, %f, %f", allBodies[i].position.x, allBodies[i].position.y, allBodies[i].position.z);

				//std::cout << TextFormat("this id: %d, other id: %d", id, allBodies[i].id);

				//char asd;
				//std::cin >> asd;
				if (CheckCollisionSpheres(bodies[i].position, bodies[i].radius, position, radius) && i != id)
				{
					if (mass > bodies[i].mass)
					{
						DrawSphere(bodies[i].position, bodies[i].radius * 5.0f, YELLOW);
						bodies.erase(bodies.begin() + i);
						/*RenderTexture2D impactedTex = LoadRenderTexture(body.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture.width, body.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture.height);
						BeginTextureMode(impactedTex);
						DrawCircle((int)Vector3Subtract(position, bodies[i].position).x, (int)Vector3Subtract(position, bodies[i].position).y, radius, ORANGE);
						EndTextureMode();

						body.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = impactedTex.texture;*/
					}
					else
					{
						DrawSphere(position, radius * 5.0f, YELLOW);
						for (std::vector<Body>::iterator iter = bodies.begin(); iter != bodies.end(); ++iter)
						{
							if (iter->id == id)
							{
								bodies.erase(iter);
								bodies.shrink_to_fit();

								/*RenderTexture2D impactedTex = LoadRenderTexture(body.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture.width, body.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture.height);
								BeginTextureMode(impactedTex);
								DrawCircle((int)Vector3Subtract(bodies[i].position, position).x, (int)Vector3Subtract(bodies[i].position, position).y, bodies[i].radius, ORANGE);
								EndTextureMode();

								bodies[i].body.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = impactedTex.texture;*/
								break;
							}
						}
					}
				}
			}
		}
	}

	bool operator == (const Body& d) const {
		return d.id == id;
	}

	bool operator != (const Body& d) const {
		return d.id != id;
	}

	void UpdatePosition(float frameTime) {
		//position = id == largestId ? position : Vector3Add(position, Vector3Scale(currentVelocity, frameTime));
		position = Vector3Add(position, Vector3Scale(currentVelocity, frameTime));

		if (frameTime != 0)
		{
			if (step < 1000)
			{
				trailPoints[step] = position;
				step++;
			}
			else
			{
				step = 0;
			}
		}

	}

	void Start(int largest, std::vector<Body>& bodies) {

		largestId = largest;

		if (id != largestId)
		{
			float orbitRad = Vector3Distance(position, bodies[largestId].position);
			initialVelocity = Vector3{ 0.0f, 0.0f,  sqrtf((G * bodies[largestId].mass) * orbitRad) };

			currentVelocity = initialVelocity;
		}
		else
		{
			
			//initialVelocity = Vector3{200.0f, 0.0f , 0.0f};
			initialVelocity = Vector3Zero();
			currentVelocity = initialVelocity;
		}
	}

	Body(int inid, float m, float rad, Vector3 pos, Color colin) {
		id = inid;

		mass = m;
		radius = rad;
		position = pos;
		initialVelocity = Vector3Zero();
		currentVelocity = Vector3Zero();
		bodCol = colin;

		if (id == 4)
		{
			body = LoadModel("spaceship2.obj");
		}
		else
		{
			Mesh tempMesh = GenMeshSphere(radius, 32, 32);
			body = LoadModelFromMesh(tempMesh);
			//UnloadMesh(tempMesh);
		}

		//Image plain = GenImageColor(32, 32, bodCol);
		Image occl = GenImageCellular(32, 32, 2);
		//Texture2D bodTex = LoadTextureFromImage(plain);
		Texture2D occTex = LoadTextureFromImage(occl);
		UnloadImage(occl);

		//body.materials[0].maps[MATERIAL_MAP_METALNESS].texture = bodTex;
		body.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = occTex;
		body.materials[0].maps[MATERIAL_MAP_DIFFUSE].color = bodCol;

	}

private:
	Vector3 currentVelocity = Vector3Zero();
	int step = 0;
	//bool operator != (const Body& d) const;
};