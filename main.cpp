/*--------------------------------------------------------------------------------------------------------------------------------------------------------------*/
/*															Vorwissenschaftliche Arbeit von Lukas Riedl															*/
/*												Darstellung dreidimensionaler Objekte auf Flächen mithilfe von Dreiecken										*/
/*																																								*/
/*															Kapitel 5.2 Darstellung einer Kugel in OpenGL														*/
/*--------------------------------------------------------------------------------------------------------------------------------------------------------------*/
/*																																								*/

/*
In diesem Kapitel werden die Daten einer Kugel, welche in blender modelliert und als "Object-File" exportiert wurde, geladen und die Daten verarbeiten. 
Der einzige Nachteil gegenüber dem Projekt in Kapitel 5.1 ist, dass .obj-DAteinen nicht das Format GL_TRINAGE_STRIP unterstützen, weshalb GL_TRIANGLES verwendet wird.
Um die Daten der Kugel zu laden wird AssImp, eine Bibliothek welche unter anderem die Rohdaten von Objekt-Dateien verarbeiten kann.

Ich habe mich außerdem dazu entschlossen, in diesem Kapitel eine Steuerung einzubauen, um dem Betrachter mehr Freiheiten zu geben. Des Weiteren sind Programmstellen, 
welche schon im letzten Kapitel erklärt wurde, nur noch mit einem kurzen Kommentar versehen oder nicht mehr erklärt.

Grob zusammengefasst kann das Kapitel in einige kleinere Aufgaben aufgeteilt werden, welche dem letzten Kapitel ähneln.

	1. Definieren der von uns benötigten mathematischen Strukturen wie Vektoren und Matrizen und deren Operationen		
	2. Erstellen einer Funktion, die Polygonnetze lädt und deren Daten korrekt speichert								
	3. OpenGL und Bibliotheken vorbereiten																				
	4. Laden der Daten																									
	5. Laden der Shader																									
	6. Durchlaufen der Grafikpipeline																					
	7. Game Loop																										
		7.1 OpenGL Einstellungen und visuelle Darstellung																
		7.2 Steuerung der Kamera																						

*/


/*--------------------------------------------------------------------------------------------------------------------------------------------------------------*/
/*																		Anfang des Programms																	*/
/*									 				0. Einbinden der benötigten Header, Bibliotheken, und Variablen												*/
/*--------------------------------------------------------------------------------------------------------------------------------------------------------------*/

#include <GL\glew.h>
#include <GLFW\glfw3.h>
#include <iostream>
#include <stdio.h>
#include <sstream>
#include <fstream>
#include <string>

#include <assert.h>
#include <assimp\cimport.h>
#include <assimp\postprocess.h>
#include <assimp\scene.h>

#define ONE_DEG_IN_RAD ( 2.0 * 3.14159265359 ) / 360.0 
#define MAX_LENGTH 262144

/*--------------------------------------------------------------------------------------------------------------------------------------------------------------*/
/*											1. Definieren von Strukturen [1] zum Generieren der benötigten Objekte (Matrix)	 									*/
/*--------------------------------------------------------------------------------------------------------------------------------------------------------------*/

// Definieren einer 4x4 Matrix
struct mat4 {
	// Konstruktoren
	mat4();
	mat4(float null, float eins, float zwei, float drei,
		float vier, float fuenf, float sechs, float sieben,
		float acht, float neun, float zehn, float elf,
		float zwoelf, float dreizehn, float vierzehn, float fuenfzehn);
	// Daten
	float e[16] = { 0 };
};

mat4::mat4() {
	e[0] = 1;
	e[1] = 0;
	e[2] = 0;
	e[3] = 0;
	e[4] = 0;
	e[5] = 1;
	e[6] = 0;
	e[7] = 0;
	e[8] = 0;
	e[9] = 0;
	e[10] = 1;
	e[11] = 0;
	e[12] = 0;
	e[13] = 0;
	e[14] = 0;
	e[15] = 1;
}
mat4::mat4(float null, float eins, float zwei, float drei,
	float vier, float fuenf, float sechs, float sieben,
	float acht, float neun, float zehn, float elf,
	float zwoelf, float dreizehn, float vierzehn, float fuenfzehn) {
	e[0] = null;
	e[1] = eins;
	e[2] = zwei;
	e[3] = drei;
	e[4] = vier;
	e[5] = fuenf;
	e[6] = sechs;
	e[7] = sieben;
	e[8] = acht;
	e[9] = neun;
	e[10] = zehn;
	e[11] = elf;
	e[12] = zwoelf;
	e[13] = dreizehn;
	e[14] = vierzehn;
	e[15] = fuenfzehn;
}

/*--------------------------------------------------------------------------------------------------------------------------------------------------------------*/
/*														2. Funktion zum laden und speichern von Polygonnetzen													*/
/*--------------------------------------------------------------------------------------------------------------------------------------------------------------*/

bool polygonnetz_laden(const char *datei_name, GLuint *vao, int *punkte_anzahl) {
	// Importieren der Datei
	const aiScene *scene = aiImportFile(datei_name, aiProcess_Triangulate);
	// Fehlerbehandlung
	if (!scene) {
		fprintf(stderr, "ERROR: reading mesh %s\n", datei_name);
		return false;
	}

	// Repräsentation unseres Modells
	const aiMesh *mesh = scene->mMeshes[0];
	*punkte_anzahl = mesh->mNumVertices;

	// Erstellen eines VAOs
	glGenVertexArrays(1, vao);
	glBindVertexArray(*vao);

	// Speicherplatz für den Inhalt des Modells
	GLfloat *punkte = NULL;
	GLfloat *normalen = NULL;

	// Wenn "mesh" Punkte (Positionen) beinhält
	if (mesh->HasPositions()) {
		// Speicherplatz reservieren
		punkte = (GLfloat *)malloc(*punkte_anzahl * 3 * sizeof(GLfloat));
		// Jede Zahl kopieren, sodass die Punkte folgendermaßen gespeichert werden:
		// { x, y, z, x, y, z, ... }
		for (int i = 0; i < *punkte_anzahl; i++) {
			const aiVector3D *vp = &(mesh->mVertices[i]);
			punkte[i * 3] = (GLfloat)vp->x;
			punkte[i * 3 + 1] = (GLfloat)vp->y;
			punkte[i * 3 + 2] = (GLfloat)vp->z;
		}
	}

	// Funktion ausführen, wenn "mesh" Normalen beinhält
	if (mesh->HasNormals()) {
		// Speicherplatz reservieren
		normalen = (GLfloat *)malloc(*punkte_anzahl * 3 * sizeof(GLfloat));
		// Jede Zahlen so kopieren, dass die Normalen folgendermaßen gespeichert werden:
		// { x, y, z, x, y, z, ... }
		for (int i = 0; i < *punkte_anzahl; i++) {
			const aiVector3D *vn = &(mesh->mNormals[i]);
			normalen[i * 3] = (GLfloat)vn->x;
			normalen[i * 3 + 1] = (GLfloat)vn->y;
			normalen[i * 3 + 2] = (GLfloat)vn->z;
		}
	}

	// Ausführen, wenn "mesh" Punkte (Positionen) beinhält
	if (mesh->HasPositions()) {
		// Erstellen eines VBOs, welches die Daten der Punkte speichert
		GLuint vbo;
		glGenBuffers(1, &vbo);
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		glBufferData(GL_ARRAY_BUFFER, 3 * *punkte_anzahl * sizeof(GLfloat), punkte, GL_STATIC_DRAW);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, NULL);
		glEnableVertexAttribArray(0);
		// Speicherplatz freigeben
		free(punkte);
	}

	// Ausführen, wenn "mesh" Normalen beinhält
	if (mesh->HasNormals()) {
		// Erstellen eines VBOs, welches die Daten der Normalen speichert
		GLuint vbo;
		glGenBuffers(1, &vbo);
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		glBufferData(GL_ARRAY_BUFFER, 3 * *punkte_anzahl * sizeof(GLfloat), normalen, GL_STATIC_DRAW);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, NULL);
		glEnableVertexAttribArray(1);
		// Speicherplatz freigben
		free(normalen);
	}

	// Speicherplatz freigeben
	aiReleaseImport(scene);
	
	return true;
}



/*--------------------------------------------------------------------------------------------------------------------------------------------------------------*/
/*																	Beginn der main-Funktion																	*/
/*--------------------------------------------------------------------------------------------------------------------------------------------------------------*/

int main() {

	/*--------------------------------------------------------------------------------------------------------------------------------------------------------------*/
	/*															3. OpenGL und Bibliotheken vorbereiten																*/
	/*--------------------------------------------------------------------------------------------------------------------------------------------------------------*/

	glfwInit();
	GLFWwindow *window = glfwCreateWindow(640, 480, "Wuerfel in OpenGL 4", NULL, NULL);
	glfwMakeContextCurrent(window);
	glewInit();
	glClearColor(0.3f, 0.3f, 0.3f, 1.0f);

	/*--------------------------------------------------------------------------------------------------------------------------------------------------------------*/
	/*																		4. Laden der Daten																		*/
	/*--------------------------------------------------------------------------------------------------------------------------------------------------------------*/

	// Vertex Buffer Objects, welche auf Daten referenzieren sollen
	GLuint vao;
	int point_count;
	// Laden der Daten
	(polygonnetz_laden("kugel.obj", &vao, &point_count));

	

	/*--------------------------------------------------------------------------------------------------------------------------------------------------------------*/
	/*																		5. Laden der Shader																		*/
	/*--------------------------------------------------------------------------------------------------------------------------------------------------------------*/
	
	std::ifstream vertex_shader_file;
	std::ifstream fragment_shader_file;
	vertex_shader_file.open("shad.vert");
	fragment_shader_file.open("shad.frag");
	std::stringstream vertex_shader_stream;
	std::stringstream fragment_shader_stream;
	vertex_shader_stream << vertex_shader_file.rdbuf();
	fragment_shader_stream << fragment_shader_file.rdbuf();
	std::string vertex_shader_data_string;
	std::string fragment_shader_data_string;
	vertex_shader_data_string = vertex_shader_stream.str();
	fragment_shader_data_string = fragment_shader_stream.str();
	const GLchar *vertex_shader_data = vertex_shader_data_string.c_str();
	const GLchar *fragment_shader_data = fragment_shader_data_string.c_str();
	
	GLuint shader_program = glCreateProgram();
	GLuint vertex_shader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertex_shader, 1, &vertex_shader_data, NULL);
	glCompileShader(vertex_shader);
	glAttachShader(shader_program, vertex_shader);
	GLuint fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragment_shader, 1, &fragment_shader_data, NULL);
	glCompileShader(fragment_shader);
	glAttachShader(shader_program, fragment_shader);
	glLinkProgram(shader_program);


	/*--------------------------------------------------------------------------------------------------------------------------------------------------------------*/
	/*																6. Durchlaufen der Grafikpipeline																*/
	/*--------------------------------------------------------------------------------------------------------------------------------------------------------------*/

	float kamera_position[] = { 0.0f, 0.0f, 2.0f };
	float kamera_geschw = 10.0f;
	float kamera_rotation_geschw = 5.0f;
	float kamera_winkel = 0.0f;
	mat4 transform_mat;
	transform_mat.e[12] = -kamera_position[0];
	transform_mat.e[13] = -kamera_position[1];
	transform_mat.e[14] = -kamera_position[2];
	mat4 rotations_mat = mat4();
	double kamera_rot_in_rad = kamera_winkel * (67.0 * ((2.0 * 3.14159265359) / 360.0));
	rotations_mat.e[0] = std::cos(kamera_rot_in_rad);
	rotations_mat.e[2] = -std::sin(kamera_rot_in_rad);
	rotations_mat.e[8] = std::sin(kamera_rot_in_rad);
	rotations_mat.e[10] = std::cos(kamera_rot_in_rad);
	mat4 view_mat;
	int index = 0;
	for (int spalte = 0; spalte < 4; spalte++) {
		for (int zeile = 0; zeile < 4; zeile++) {
			float sum = 0.0f;
			for (int i = 0; i < 4; i++) {
				sum += rotations_mat.e[zeile + i * 4] * transform_mat.e[i + spalte * 4];
			}
			view_mat.e[index] = sum;
			index++;
		}
	}

	// Definieren der benötigte Daten
	float naheEbene = 1.0f;
	float ferneEbene = 100.0f;
	float sv = (float)640 / (float)480;
	// Field of View in Radiant
	float fov = 90.0 * (3.14159265359 / 180.0);
	float d = 1 / tan(fov / 2);

	// Zusammenstellen der Projection Matrix
	mat4 proj_mat;
	proj_mat.e[0] = 1 / (sv * tan(fov / 2));
	proj_mat.e[5] = d;
	proj_mat.e[10] = (-naheEbene - ferneEbene) / (naheEbene - ferneEbene);
	proj_mat.e[11] = (2.0f * ferneEbene * naheEbene) / (naheEbene - ferneEbene);
	proj_mat.e[14] = 1.0f;

	GLint view_mat_location = glGetUniformLocation(shader_program, "view");
	glUseProgram(shader_program);
	glUniformMatrix4fv(view_mat_location, 1, GL_FALSE, view_mat.e);

	GLint proj_mat_location = glGetUniformLocation(shader_program, "proj");
	glUseProgram(shader_program);
	glUniformMatrix4fv(proj_mat_location, 1, GL_FALSE, proj_mat.e);


	/*--------------------------------------------------------------------------------------------------------------------------------------------------------------*/
	/*																		7. Game-Loop																			*/
	/*--------------------------------------------------------------------------------------------------------------------------------------------------------------*/


	while (!glfwWindowShouldClose(window)) {

		/*--------------------------------------------------------------------------------------------------------------------------------------------------------------*/
		/*														7.1 OpenGL Einstellungen und visuelle Darstellung 														*/
		/*--------------------------------------------------------------------------------------------------------------------------------------------------------------*/
		
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

		glViewport(0, 0, 640, 480);
		glUseProgram(shader_program);
		glBindVertexArray(vao);

		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_LESS);
		glDrawArrays(GL_TRIANGLES, 0, point_count);
		glfwPollEvents();

		/*--------------------------------------------------------------------------------------------------------------------------------------------------------------*/
		/*																	7.2 Steuerung der Kamera 																	*/
		/*--------------------------------------------------------------------------------------------------------------------------------------------------------------*/

		// Variable zum überprüfen, ob sich die Kamera bewegt hat
		bool cam_moved = false;

		// Berechnen der Zeit zwischen dem momentanen und dem vorherigen Frame
		static double vorherige_zeit = glfwGetTime();
		double momentane_zeit = glfwGetTime();
		double vergangene_zeit = momentane_zeit - vorherige_zeit;
		vorherige_zeit = momentane_zeit;

		// Extrahieren wichtiger Daten aus der View Matrix
		float forward_vec[] = { (float)-view_mat.e[2],(float)-view_mat.e[6], (float)-view_mat.e[10] };
		float right_vec[] = { (float)-view_mat.e[0],(float)-view_mat.e[4], (float)-view_mat.e[8] };

		// Folgende if-Statements prüfen auf mögliche gedrückte Tasten und modifiziert dementsprechend die Position der Kamera
		if (glfwGetKey(window, GLFW_KEY_W)) {
			if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT)) {
				kamera_position[0] -= 3 * (forward_vec[0]) * kamera_geschw * (float)vergangene_zeit;
				kamera_position[1] -= 3 * (forward_vec[1]) * kamera_geschw * (float)vergangene_zeit;
				kamera_position[2] += 3 * (forward_vec[2]) * kamera_geschw * (float)vergangene_zeit;
			}
			else {
				kamera_position[0] -= (forward_vec[0]) * kamera_geschw * (float)vergangene_zeit;
				kamera_position[1] -= (forward_vec[1]) * kamera_geschw * (float)vergangene_zeit;
				kamera_position[2] += (forward_vec[2]) * kamera_geschw * (float)vergangene_zeit;
			}
			cam_moved = true;
		}
		if (glfwGetKey(window, GLFW_KEY_S)) {
			kamera_position[0] += (forward_vec[0]) * kamera_geschw * (float)vergangene_zeit;
			kamera_position[1] += (forward_vec[1]) * kamera_geschw * (float)vergangene_zeit;
			kamera_position[2] -= (forward_vec[2]) * kamera_geschw * (float)vergangene_zeit;
			cam_moved = true;
		}
		if (glfwGetKey(window, GLFW_KEY_D)) {
			kamera_position[0] += (right_vec[0]) * kamera_geschw * (float)vergangene_zeit;
			kamera_position[1] -= (right_vec[1]) * kamera_geschw * (float)vergangene_zeit;
			kamera_position[2] -= (right_vec[2]) * kamera_geschw * (float)vergangene_zeit;
			cam_moved = true;
		}
		if (glfwGetKey(window, GLFW_KEY_A)) {
			kamera_position[0] -= (right_vec[0]) * kamera_geschw * (float)vergangene_zeit;
			kamera_position[1] += (right_vec[1]) * kamera_geschw * (float)vergangene_zeit;
			kamera_position[2] += (right_vec[2]) * kamera_geschw * (float)vergangene_zeit;
			cam_moved = true;
		}


		if (glfwGetKey(window, GLFW_KEY_UP)) {
			kamera_position[1] += kamera_geschw * (float)vergangene_zeit;
			cam_moved = true;
		}
		if (glfwGetKey(window, GLFW_KEY_DOWN)) {
			kamera_position[1] -= kamera_geschw * (float)vergangene_zeit;
			cam_moved = true;
		}


		if (glfwGetKey(window, GLFW_KEY_LEFT)) {
			kamera_winkel += kamera_rotation_geschw * (float)vergangene_zeit;
			cam_moved = true;
		}
		if (glfwGetKey(window, GLFW_KEY_RIGHT)) {
			kamera_winkel -= kamera_rotation_geschw * (float)vergangene_zeit;
			cam_moved = true;
		}

		// Wenn "ESC" gedrückt wird, soll sich das Fenster schließen
		if (glfwGetKey(window, GLFW_KEY_ESCAPE)) {
			glfwSetWindowShouldClose(window, GL_TRUE);
		}

		// Ausführen, sollte sich die Kamera in dieser Schleife verändert haben
		if (cam_moved) {
			// Modifizieren der Transformationsmatrix
			transform_mat = mat4();
			transform_mat.e[12] = kamera_position[0];
			transform_mat.e[13] = -kamera_position[1];
			transform_mat.e[14] = -kamera_position[2];
			// Modifizieren der Rotationsmatrix
			rotations_mat = mat4();
			rotations_mat.e[0] = std::cos(-kamera_winkel);
			rotations_mat.e[2] = -std::sin(-kamera_winkel);
			rotations_mat.e[8] = std::sin(-kamera_winkel);
			rotations_mat.e[10] = std::cos(-kamera_winkel);

			// Aktualisieren der View Matrix
			view_mat = mat4();
			index = 0;
			for (int spalte = 0; spalte < 4; spalte++) {
				for (int zeile = 0; zeile < 4; zeile++) {
					float sum = 0.0f;
					for (int i = 0; i < 4; i++) {
						sum += rotations_mat.e[zeile + i * 4] * transform_mat.e[i + spalte * 4];
					}
					view_mat.e[index] = sum;
					index++;
				}
			}
			// Aktualisieren der View Matrix im Shader
			glUniformMatrix4fv(view_mat_location, 1, GL_FALSE, view_mat.e);
		}
		glfwSwapBuffers(window);
	}
	glfwTerminate();
	return 0;
}


/*

[1] Strukturen sind eigendefinierte Datentypen, welche ein oder mehrere Datentypen zu einem Typen zusammengefassen.


Quellenverzeichnis: 
	http://www.opengl-tutorial.org/beginners-tutorials/tutorial-7-model-loading/
	Gerdelan Anton: Anton's OpenGL4 Tutorials[E-Book]. Amazon Digital Services LLC. 2014
*/