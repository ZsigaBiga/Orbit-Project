#pragma once

float GetRandomValueF(float min, float max) {
	return min + static_cast <unsigned> (rand()) / (static_cast <float> (RAND_MAX / (max - min)));
}

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
				Vector3 acceleration = Vector3Scale(force, (1.0f / mass));

				forceOut = currentVelocity;
				currentVelocity = Vector3Add(currentVelocity, Vector3Scale(acceleration, timestep));

				if (CheckCollisionSpheres(bodies[i].position, bodies[i].radius, position, radius) && i != id)
				{
					if (mass > bodies[i].mass)
					{
						DrawSphere(bodies[i].position, bodies[i].radius * 5.0f, YELLOW);
						bodies.erase(bodies.begin() + i);
						
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

	void Start(int largest, std::vector<Body>& bodies, int procNum) {

		largestId = largest;

		if (id != largestId)
		{
			float orbitRad = Vector3Distance(position, bodies[largestId].position);
			float ooblify = procNum == 2 ? 1.0f : GetRandomValueF(0.65f, 1.0f);
			initialVelocity = Vector3{ 0.0f, 0.0f,  sqrtf((G * bodies[largestId].mass) * orbitRad) * ooblify};

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

		if (id == 4 && m < 3.38f)
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
		Image occl = GenImagePerlinNoise(32, 32, 0, 0, 1.0f);
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