//Ajout

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/signalfd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <netdb.h>
#include <signal.h>
#include <inttypes.h>

#include <gtk/gtk.h>

//ajout de la librairie pour utilisation de boolean
#include <stdbool.h>

#define MAXDATASIZE 256

#define TAILLE_DAMIER 8			//8 lignes, 8 colonnes
#define EN_DEHORS_TAB -2
#define VIDE -1
#define NOIR 0
#define BLANC 1

#define TYPE_COULEUR 0
#define TYPE_COORD 1
#define TYPE_IMPOSSIBILITE_COUP 2
#define TYPE_FIN 3
// Fin ajout

struct requete {
	uint16_t type;				//0 pour la couleur, 1 pour les coordonnées x, y
	uint16_t x;
	uint16_t y;
	uint16_t couleur;
};

/* Variables globales */
int damier[TAILLE_DAMIER][TAILLE_DAMIER];	// tableau associe au damier
int couleur;

int port;						// numero port passe a l'appel

char *addr_j2, *port_j2;		// Info sur adversaire

// Ajout

pthread_t thr_id;				// Id du thread fils gerant connexion socket

int sockfd, newsockfd = -1;		// descripteurs de socket
int addr_size;					// taille adresse
struct sockaddr *their_addr;	// structure pour stocker adresse adversaire

fd_set master, read_fds, write_fds;	// ensemble de socket pour toutes les sockets actives avec select
int fdmax;						// utilise pour select

int status;
struct addrinfo hints, *servinfo, *p;

// Fin ajout

/* Variables globales associées à l'interface graphique */
GtkBuilder *p_builder = NULL;
GError *p_err = NULL;


/** DEBUT DES ENTÊTES DES FONCTIONS **/

/**
* Fonction permettant de changer l'image d'une case du damier
* (indiqué par sa colonne et sa ligne)
* @param col la colonne de la case du damier
* @param lig la ligne de la case du damier
* @param couleur_j la couleur du joueur (0 noir, 1 blanc)
*/
void change_img_case(int col, int lig, int couleur_j);

/**
* Fonction permettant changer nom joueur blanc dans cadre Score
* @param texte nom du J1 (blanc)
* @usefulfor interface graphique
*/
void set_label_J1(char *texte);

/**
* Fonction permettant changer nom joueur noir dans cadre Score
* @param texte nom du J2 (noir)
* @usefulfor interface graphique
*/
void set_label_J2(char *texte);

/**
* Fonction permettant de changer score joueur blanc dans cadre Score
* @param score le score du J1 (blanc)
* @see get_score_J1(void)
*/
void set_score_J1(int score);

/**
* Fonction permettant de récupérer score joueur blanc dans cadre Score
* @return le score du J1 (blanc)
* @see set_score_J1(int)
*/
int get_score_J1(void);

/**
* Fonction permettant de changer score joueur noir dans cadre Score
* @param score le score du J2 (noir)
* @see #get_score_J2(void)
*/
void set_score_J2(int score);

/**
* Fonction permettant de récupérer score joueur noir dans cadre Score
* @return le score du J2 (noir)
* @see #set_score_J2(int)
*/
int get_score_J2(void);

/**
* Fonction transformant les coordonnees du damier graphique
* en indexes pour matrice du damier
*/
void coord_to_indexes(const gchar * coord, int *col, int *lig);

/**
* Fonction transformant les indexes pour matrice
* du damier en coordonnees du damier graphique
*/
void indexes_to_coord(int col, int lig, char *coord);

/**
* Fonction appelee lors du clic
* sur une case du damier
*/
static void coup_joueur(GtkWidget * p_case);

/**
* Fonction retournant texte du champs adresse
* du serveur de l'interface graphique
* @return l'adresse du serveur
*/
char *lecture_addr_serveur(void);

/**
* Fonction retournant le texte du champs port
* du serveur de l'interface graphique
* @return le port du serveur
*/
char *lecture_port_serveur(void);

/**
* Fonction retournant le texte du champs login
* de l'interface graphique
* @return le login
*/
char *lecture_login(void);

/**
* Fonction retournant le texte du champs adresse
* du cadre Joueurs de l'interface graphique
* @return l'adresse de l'adversaire
*/
char *lecture_addr_adversaire(void);

/**
* Fonction retournant le texte du champs port
* du cadre Joueurs de l'interface graphique
* @return le port de l'adversaire
*/
char *lecture_port_adversaire(void);

/**
* Fonction permettant d'afficher
* soit #affiche_fenetre_gagne(void)
* soit #affiche_fenetre_perdu(void)
* en fonction des cas et en fonction
* de la couleur qui jouer
*/
void affiche_fenetre(void);

/**
* Fonction affichant une boite de dialogue
* si la partie est gagnée
* @usefulfor interface graphique
*/
void affiche_fenetre_gagne(void);

/**
* Fonction affichant une boite de dialogue
* si la partie est perdue
* @usefulfor interface graphique
*/
void affiche_fenetre_perdu(void);

/**
* Fonction affichant une boite de dialogue
* si le joueur ne peut pas jouer
* @usefulfor interface graphique
*/
void affiche_fenetre_impossible_jouer(void);

/**
* Fonction appelée lors du clic du bouton Se connecter
* @usefulfor interface graphique
*/
static void clique_connect_serveur(GtkWidget * b);

/**
* Fonction desactivant bouton demarrer partie
* @usefulfor interface graphique
*/
void disable_button_start(void);

/**
* Fonction appelee lors du clic du bouton Demarrer partie
* @usefulfor interface graphique
*/
static void clique_connect_adversaire(GtkWidget * b);

/**
* Fonction desactivant les cases du damier
* pour attendre que l'utilisateur adverse joue (ignore les clics du joueurs courant)
* @usefulfor interface graphique
* @see #degele_damier(void)
*/
void gele_damier(void);

/**
* Fonction activant les cases du damier
* pour que l'utilisateur puisse jouer
* @usefulfor interface graphique
* @see #gele_damier(void)
*/
void degele_damier(void);

/**
* Fonction permettant d'initialiser le plateau de jeu
* @usefulfor interface graphique
*/
void init_interface_jeu(void);

/**
* Fonction reinitialisant la liste des joueurs sur l'interface graphique
* @usefulfor interface graphique
*/
void reset_liste_joueurs(void);

/**
* Fonction permettant d'ajouter un joueur
* dans la liste des joueurs sur l'interface graphique
* @param login le login du joueur
* @param adresse l'adresse ip du joueur
* @param port le port du joueur
* @usefulfor interface graphique
*/
void affich_joueur(char *login, char *adresse, char *port);

/**
* Fonction exécutée par le thread gérant
* les communications à travers la socket
*/
static void *f_com_socket(void *p_arg);

//ajout des entetes qui ne sont pas du prof

/**
* Fonction vérifiant si une case est jouable
* 1/ en vérifiant qu'on ne depasse pas la taille du damier
* 2/ que l'on joue en se basant sur son voisin
*/
bool jouable(int col, int lig, int couleur_joueur);

/**
* Fonction permettant de changer la couleur de la case
*/
int set_case(int colonne, int ligne, int couleur);

/**
* Fonction qui retourne si on doit changer la ligne
*/
bool change_ligne(int colonne, int ligne, int chmtx, int chmty, int couleur);

/**
* Fonction changeant la couleur des cases
* @see #changement_case(int, int, int, int, int)
*/
void changements_case(int colonne, int ligne, int couleur);

/**
* Fonction changeant la couleur des cases dans la direction choisie
*/
void changement_case(int colonne, int ligne, int chmtx, int chmty, int couleur);

/**
* Fonction permettant de savoir
* si on peut encore placer des pièces
*/
bool partie_finie();

/**
* Fonction permettant de savoir
* si les deux joueurs ne peuvent poser
* plus de pièces
* @see http://www.ffothello.org/images/jeu/regles/figure-10.jpg
*/
bool fin_prematuree();

/**
* Fonction permettant de savoir
* si le joueur ne peut pas jouer
*/
bool impossible_jouer();

/**
* Fonction permettant de retourner la couleur de la case
*/
int return_couleur_case(int colonne, int ligne);

/**
* Fonction retournant la couleur du joueur courant
*/
int get_couleur();

/**
* Fonction initialisant la couleur du joueur courant
*/
void set_couleur(int couleur_joueur);

/**
* Fonction retournant la couleur de l'adversaire
*/
int get_couleur_adversaire();

/**
* Fonction retournant la couleur adverse
* @param la couleur
* @return la couleur adverse à @param
*/
int get_couleur_adverse(int couleur);


/** FIN DES ENTÊTES DES FONCTIONS **/

/**
* Fonction transformant les coordonnees du damier graphique
* en indexes pour matrice du damier
*/
void coord_to_indexes(const gchar * coord, int *col, int *lig)
{
	char *c;
	c = malloc(3 * sizeof(char));

	c = strncpy(c, coord, 1);
	c[1] = '\0';

	if (strcmp(c, "A") == 0)
		*col = 0;
	else if (strcmp(c, "B") == 0)
		*col = 1;
	else if (strcmp(c, "C") == 0)
		*col = 2;
	else if (strcmp(c, "D") == 0)
		*col = 3;
	else if (strcmp(c, "E") == 0)
		*col = 4;
	else if (strcmp(c, "F") == 0)
		*col = 5;
	else if (strcmp(c, "G") == 0)
		*col = 6;
	else if (strcmp(c, "H") == 0)
		*col = 7;

	*lig = atoi(coord + 1) - 1;
}

/**
* Fonction transformant les indexes pour matrice
* du damier en coordonnees du damier graphique
*/
void indexes_to_coord(int col, int lig, char *coord)
{
	char c;

	if (col == 0)
		c = 'A';
	else if (col == 1)
		c = 'B';
	else if (col == 2)
		c = 'C';
	else if (col == 3)
		c = 'D';
	else if (col == 4)
		c = 'E';
	else if (col == 5)
		c = 'F';
	else if (col == 6)
		c = 'G';
	else if (col == 7)
		c = 'H';

	sprintf(coord, "%c%d\0", c, lig + 1);
}

// Fin ajout

/**
* Fonction permettant de changer l'image d'une case du damier
* (indiqué par sa colonne et sa ligne)
* @param col la colonne de la case du damier
* @param lig la ligne de la case du damier
* @param couleur_j la couleur du joueur (0 noir, 1 blanc)
*/
void change_img_case(int col, int lig, int couleur_j)
{
	char *coord;

	coord = malloc(3 * sizeof(char));

	indexes_to_coord(col, lig, coord);

	if (couleur_j == BLANC)
	{
		// image pion blanc
		gtk_image_set_from_file(GTK_IMAGE
								(gtk_builder_get_object(p_builder, coord)),
								"UI_Glade/case_blanc.png");
	}
	else
	{
		// image pion noir
		gtk_image_set_from_file(GTK_IMAGE
								(gtk_builder_get_object(p_builder, coord)),
								"UI_Glade/case_noir.png");
	}
}

/**
* Fonction permettant changer nom joueur blanc dans cadre Score
* @param texte nom du J1 (blanc)
* @usefulfor interface graphique
*/
void set_label_J1(char *texte)
{
	gtk_label_set_text(GTK_LABEL(gtk_builder_get_object(p_builder, "label_J1")),
					   texte);
}

/**
* Fonction permettant changer nom joueur noir dans cadre Score
* @param texte nom du J2 (noir)
* @usefulfor interface graphique
*/
void set_label_J2(char *texte)
{
	gtk_label_set_text(GTK_LABEL(gtk_builder_get_object(p_builder, "label_J2")),
					   texte);
}

/**
* Fonction permettant de changer score joueur blanc dans cadre Score
* @param score le score du J1 (blanc)
* @see #get_score_J1(void)
*/
void set_score_J1(int score)
{
	char *s;

	s = malloc(5 * sizeof(char));
	sprintf(s, "%d\0", score);

	gtk_label_set_text(GTK_LABEL
					   (gtk_builder_get_object(p_builder, "label_ScoreJ1")), s);
}

/**
* Fonction permettant de récupérer score joueur blanc dans cadre Score
* @return le score du J1 (blanc)
* @see #set_score_J1(int)
*/
int get_score_J1(void)
{
	const gchar *c;

	c = gtk_label_get_text(GTK_LABEL
						   (gtk_builder_get_object
							(p_builder, "label_ScoreJ1")));

	return atoi(c);
}

/**
* Fonction permettant de changer score joueur noir dans cadre Score
* @param score le score du J2 (noir)
* @see #get_score_J2(void)
*/
void set_score_J2(int score)
{
	char *s;

	s = malloc(5 * sizeof(char));
	sprintf(s, "%d\0", score);

	gtk_label_set_text(GTK_LABEL
					   (gtk_builder_get_object(p_builder, "label_ScoreJ2")), s);
}

/**
* Fonction permettant de récupérer score joueur noir dans cadre Score
* @return le score du J2 (noir)
* @see #set_score_J2(int)
*/
int get_score_J2(void)
{
	const gchar *c;

	c = gtk_label_get_text(GTK_LABEL
						   (gtk_builder_get_object
							(p_builder, "label_ScoreJ2")));

	return atoi(c);
}

/**
* Fonction vérifiant si une case est jouable
* 1/ en vérifiant qu'on ne depasse pas la taille du damier
* 2/ que l'on joue en se basant sur son voisin
*/
bool jouable(int col, int lig, int couleur_joueur)
{
	return (return_couleur_case(col, lig) == VIDE
			&& (change_ligne(col, lig, -1, -1, couleur_joueur)
				|| change_ligne(col, lig, 0, -1, couleur_joueur)
				|| change_ligne(col, lig, 1, -1, couleur_joueur)
				|| change_ligne(col, lig, -1, 0, couleur_joueur)
				|| change_ligne(col, lig, 1, 0, couleur_joueur)
				|| change_ligne(col, lig, -1, 1, couleur_joueur)
				|| change_ligne(col, lig, 0, 1, couleur_joueur)
				|| change_ligne(col, lig, 1, 1, couleur_joueur)));
}

/**
* Fonction permettant de changer la couleur de la case
*/
int set_case(int colonne, int ligne, int couleur_joueur)
{
	//on test la couleur de la case
	if (get_couleur_adverse(couleur_joueur) ==
		return_couleur_case(colonne, ligne))
	{
		if (couleur_joueur == BLANC)
		{
			set_score_J1(get_score_J1() + 1);
			set_score_J2(get_score_J2() - 1);
		}
		else if (couleur_joueur == NOIR)
		{
			set_score_J1(get_score_J1() - 1);
			set_score_J2(get_score_J2() + 1);
		}

	}
	else
	{
		if (couleur_joueur == BLANC)
		{
			set_score_J1(get_score_J1() + 1);
		}
		else if (couleur_joueur == NOIR)
		{
			set_score_J2(get_score_J2() + 1);
		}
	}
	// On insere le pion de la couleur du joueur courant
	damier[colonne][ligne] = couleur_joueur;
	printf("changement couleur de %d;%d en %d\n", colonne, ligne,
		   couleur_joueur);
	//On change l'image de la case en transformant son numéro en img
	change_img_case(colonne, ligne, couleur_joueur);
}



bool change_ligne(int colonne, int ligne, int chmtx, int chmty, int couleur)
{
	int compteur = 0;
	// on fait un parcours tant que la couleur de la selection de direction est différente
	colonne += chmtx;
	ligne += chmty;
	while (return_couleur_case(colonne, ligne) == get_couleur_adverse(couleur))
	{
		//on increment le compteur
		compteur++;
		//on increment le parcours vers la direction choisie
		colonne += chmtx;
		ligne += chmty;
	}
	//si la couleur qu'on vient de parcourir est la même que la couleur du joueur qui vient de jouer ET que le compteur est supéreur a 1(pour dire que c'est pas un voisin de la meme couleur)
	return ((return_couleur_case(colonne, ligne) == couleur) && (compteur > 0));
}

/**
* Fonction changeant la couleur des cases
* @see #changement_case(int, int, int, int, int)
*/
void changements_case(int colonne, int ligne, int couleur)
{
	if (change_ligne(colonne, ligne, -1, -1, couleur))
	{
		changement_case(colonne, ligne, -1, -1, couleur);
	}
	if (change_ligne(colonne, ligne, 0, -1, couleur))
	{
		changement_case(colonne, ligne, 0, -1, couleur);
	}
	if (change_ligne(colonne, ligne, 1, -1, couleur))
	{
		changement_case(colonne, ligne, 1, -1, couleur);
	}
	if (change_ligne(colonne, ligne, -1, 0, couleur))
	{
		changement_case(colonne, ligne, -1, 0, couleur);
	}
	if (change_ligne(colonne, ligne, 1, 0, couleur))
	{
		changement_case(colonne, ligne, 1, 0, couleur);
	}
	if (change_ligne(colonne, ligne, -1, 1, couleur))
	{
		changement_case(colonne, ligne, -1, 1, couleur);
	}
	if (change_ligne(colonne, ligne, 0, 1, couleur))
	{
		changement_case(colonne, ligne, 0, 1, couleur);
	}
	if (change_ligne(colonne, ligne, 1, 1, couleur))
	{
		changement_case(colonne, ligne, 1, 1, couleur);
	}
}

/**
* Fonction changeant la couleur des cases dans la direction choisie
*/
void changement_case(int colonne, int ligne, int chmtx, int chmty, int couleur)
{

	colonne += chmtx;
	ligne += chmty;
	while (return_couleur_case(colonne, ligne) == get_couleur_adverse(couleur))
	{
		// on change la couleur de la case et on met à jour le score
		set_case(colonne, ligne, couleur);
		//on increment le parcours vers la direction choisie
		colonne += chmtx;
		ligne += chmty;
	}
}

/**
* Fonction permettant de savoir
* si on peut encore placer des pièces
*/
bool partie_finie()
{
	int j1, j2, limitescore;
	j1 = get_score_J1();
	j2 = get_score_J2();
	// la limite est le nb de points total pour savoir si le jeu est fini
	limitescore = TAILLE_DAMIER * TAILLE_DAMIER;
	return ((j1 + j2) == limitescore);
}

/**
* Fonction permettant de savoir
* si les deux joueurs ne peuvent poser
* plus de pièces
* @see http://www.ffothello.org/images/jeu/regles/figure-10.jpg
*/
bool fin_prematuree()
{
	int i, j;
	for (i = 0; i < TAILLE_DAMIER; ++i)
	{
		for (j = 0; j < TAILLE_DAMIER; ++j)
		{
			if ((return_couleur_case(i, j) == VIDE)
				&& (jouable(i, j, BLANC) || jouable(i, j, NOIR)))
				return false;
		}
	}
	return true;
}

/**
* Fonction permettant de savoir
* si le joueur ne peut pas jouer
*/
bool impossible_jouer()
{
	int i, j;
	for (i = 0; i < TAILLE_DAMIER; ++i)
	{
		for (j = 0; j < TAILLE_DAMIER; ++j)
		{
			if ((return_couleur_case(i, j) == VIDE)
				&& (jouable(i, j, get_couleur())))
				return false;
		}
	}
	return true;
}

/**
* Fonction permettant de retourner la couleur de la case
*/
int return_couleur_case(int colonne, int ligne)
{
	if ((colonne < 0) || (ligne < 0) || (colonne >= TAILLE_DAMIER)
		|| (ligne >= TAILLE_DAMIER))
	{
		return EN_DEHORS_TAB;
	}
	else
	{
		return (damier[colonne][ligne]);
	}
}

/**
* Fonction retournant la couleur du joueur courant
*/
int get_couleur()
{
	return couleur;
}

/**
* Fonction initialisant la couleur du joueur courant
*/
void set_couleur(int couleur_joueur)
{
	couleur = couleur_joueur;
}

/**
* Fonction retournant la couleur de l'adversaire
*/
int get_couleur_adversaire()
{
	if (get_couleur() == BLANC)
	{
		return NOIR;
	}
	if (get_couleur() == NOIR)
	{
		return BLANC;
	}
}

/**
* Fonction retournant la couleur adverse
* @param la couleur
* @return la couleur adverse à @param
*/
int get_couleur_adverse(int couleur)
{
	if (couleur == BLANC)
	{
		return NOIR;
	}
	if (couleur == NOIR)
	{
		return BLANC;
	}
}

/**
* Fonction appelee lors du clic
* sur une case du damier
*/
static void coup_joueur(GtkWidget * p_case)
{
	int col, lig, type_msg, nb_piece, score;
	char buf[MAXDATASIZE];

	//ajout initiasation requete
	struct requete req;

	// Traduction coordonnees damier en indexes matrice damier
	coord_to_indexes(gtk_buildable_get_name
					 (GTK_BUILDABLE(gtk_bin_get_child(GTK_BIN(p_case)))), &col,
					 &lig);

	//envoie

	// on test si c'est jouable
	// On insere le pion de la couleur du joueur courant
	//si bien reçu FONCTION foutre pion dedans ; changer les autres pions en regardant depuis le point du pion en diagonale/etc jusqu'à trouver un de nos pions.  on continue tant qu'on a pion adverse tant on dépasse pas le tableau

	if (partie_finie())
	{
		//les joueurs ne peuvent pas jouer car le jeu est rempli
		//donc partie finie
		gele_damier();

		//préparation message et envoie sur socket à adversaire
		req.type = htons((uint16_t) TYPE_FIN);

		printf("Requête à envoyer %u\n", req.type);
		bzero(buf, MAXDATASIZE);
		snprintf(buf, MAXDATASIZE, "%u", req.type);

		printf("Envoyer requête %s\n", buf);
		if (send(newsockfd, &buf, strlen(buf), 0) == -1)
		{
			perror("send");
			close(newsockfd);
		}

		affiche_fenetre();
	}
	else if (fin_prematuree())
	{
		// les deux joueurs ne peuvent pas placer le pion
		// donc partie finie
		// les places vident vont au gagnant
		int place_vide =
			(TAILLE_DAMIER * TAILLE_DAMIER) - (get_score_J1() + get_score_J2());
		if (get_score_J1() > get_score_J2())
		{
			set_score_J1(get_score_J1() + place_vide);
		}
		else
		{
			set_score_J2(get_score_J2() + place_vide);
		}

		//préparation message et envoie sur socket à adversaire
		req.type = htons((uint16_t) TYPE_FIN);

		printf("Requête à envoyer %u\n", req.type);
		bzero(buf, MAXDATASIZE);
		snprintf(buf, MAXDATASIZE, "%u", req.type);

		printf("Envoyer requête %s\n", buf);
		if (send(newsockfd, &buf, strlen(buf), 0) == -1)
		{
			perror("send");
			close(newsockfd);
		}


		affiche_fenetre();
	}
	else if (impossible_jouer())
	{
		//le joueur ne peut pas placer le pion mais l'autre si

		gele_damier();

		//préparation message et envoie sur socket à adversaire
		req.type = htons((uint16_t) TYPE_IMPOSSIBILITE_COUP);

		printf("Requête à envoyer %u\n", req.type);
		bzero(buf, MAXDATASIZE);
		snprintf(buf, MAXDATASIZE, "%u", req.type);

		printf("Envoyer requête %s\n", buf);
		if (send(newsockfd, &buf, strlen(buf), 0) == -1)
		{
			perror("send");
			close(newsockfd);
		}

		affiche_fenetre_impossible_jouer();
	}
	else
	{
		if (jouable(col, lig, get_couleur()))
		{
			//change la case
			set_case(col, lig, get_couleur());
			gele_damier();
			//déterminer pions dont la couleur modifie + score
			changements_case(col, lig, get_couleur());

			printf("Info à envoyer : %u %u\n", col, lig);

			//préparation message et envoie sur socket à adversaire
			req.type = htons((uint16_t) TYPE_COORD);
			req.x = htons((uint16_t) col);
			req.y = htons((uint16_t) lig);
			printf("Requête à envoyer %u : %u %u\n", req.type, req.x, req.y);
			bzero(buf, MAXDATASIZE);
			snprintf(buf, MAXDATASIZE, "%u, %u, %u", req.type, req.x, req.y);

			printf("Envoyer requête %s\n", buf);
			if (send(newsockfd, &buf, strlen(buf), 0) == -1)
			{
				perror("send");
				close(newsockfd);
			}
		}
	}
}

/**
* Fonction retournant texte du champs adresse
* du serveur de l'interface graphique
* @return l'adresse du serveur
*/
char *lecture_addr_serveur(void)
{
	GtkWidget *entry_addr_srv;

	entry_addr_srv =
		(GtkWidget *) gtk_builder_get_object(p_builder, "entry_adr");

	return (char *) gtk_entry_get_text(GTK_ENTRY(entry_addr_srv));
}

/**
* Fonction retournant le texte du champs port
* du serveur de l'interface graphique
* @return le port du serveur
*/
char *lecture_port_serveur(void)
{
	GtkWidget *entry_port_srv;

	entry_port_srv =
		(GtkWidget *) gtk_builder_get_object(p_builder, "entry_port");

	return (char *) gtk_entry_get_text(GTK_ENTRY(entry_port_srv));
}

/**
* Fonction retournant le texte du champs login
* de l'interface graphique
* @return le login
*/
char *lecture_login(void)
{
	GtkWidget *entry_login;

	entry_login =
		(GtkWidget *) gtk_builder_get_object(p_builder, "entry_login");

	return (char *) gtk_entry_get_text(GTK_ENTRY(entry_login));
}

/**
* Fonction retournant le texte du champs adresse
* du cadre Joueurs de l'interface graphique
* @return l'adresse de l'adversaire
*/
char *lecture_addr_adversaire(void)
{
	GtkWidget *entry_addr_j2;

	entry_addr_j2 =
		(GtkWidget *) gtk_builder_get_object(p_builder, "entry_addr_j2");

	return (char *) gtk_entry_get_text(GTK_ENTRY(entry_addr_j2));
}

/**
* Fonction retournant le texte du champs port
* du cadre Joueurs de l'interface graphique
* @return le port de l'adversaire
*/
char *lecture_port_adversaire(void)
{
	GtkWidget *entry_port_j2;

	entry_port_j2 =
		(GtkWidget *) gtk_builder_get_object(p_builder, "entry_port_j2");

	return (char *) gtk_entry_get_text(GTK_ENTRY(entry_port_j2));
}

/**
* Fonction permettant d'afficher
* soit #affiche_fenetre_gagne(void)
* soit #affiche_fenetre_perdu(void)
* en fonction des cas et en fonction
* de la couleur qui jouer
*/
void affiche_fenetre(void)
{
	if (get_couleur() == BLANC)
	{
		if (get_score_J1() > get_score_J2())
		{
			affiche_fenetre_gagne();
		}
		else
		{
			affiche_fenetre_perdu();
		}
	}
	else if (get_couleur() == NOIR)
	{
		if (get_score_J2() > get_score_J1())
		{
			affiche_fenetre_gagne();
		}
		else
		{
			affiche_fenetre_perdu();
		}
	}
}

/**
* Fonction affichant une boite de dialogue
* si la partie est gagnée
* @usefulfor interface graphique
*/
void affiche_fenetre_gagne(void)
{
	GtkWidget *dialog;

	GtkDialogFlags flags = GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT;

	dialog =
		gtk_message_dialog_new(GTK_WINDOW
							   (gtk_builder_get_object(p_builder, "window1")),
							   flags, GTK_MESSAGE_INFO, GTK_BUTTONS_CLOSE,
							   "Fin de la partie.\n\n Vous avez gagné!!!",
							   NULL);

	gtk_dialog_run(GTK_DIALOG(dialog));

	gtk_widget_destroy(dialog);
}

/**
* Fonction affichant une boite de dialogue
* si la partie est perdue
* @usefulfor interface graphique
*/
void affiche_fenetre_perdu(void)
{
	GtkWidget *dialog;

	GtkDialogFlags flags = GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT;

	dialog =
		gtk_message_dialog_new(GTK_WINDOW
							   (gtk_builder_get_object(p_builder, "window1")),
							   flags, GTK_MESSAGE_INFO, GTK_BUTTONS_CLOSE,
							   "Fin de la partie.\n\n Vous avez perdu!", NULL);

	gtk_dialog_run(GTK_DIALOG(dialog));

	gtk_widget_destroy(dialog);
}

/**
* Fonction affichant une boite de dialogue
* si le joueur ne peut pas jouer
* @usefulfor interface graphique
*/
void affiche_fenetre_impossible_jouer(void)
{
	GtkWidget *dialog;

	GtkDialogFlags flags = GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT;

	dialog =
		gtk_message_dialog_new(GTK_WINDOW
							   (gtk_builder_get_object(p_builder, "window1")),
							   flags, GTK_MESSAGE_INFO, GTK_BUTTONS_CLOSE,
							   "Impossible de jouer.\n\n :)", NULL);

	gtk_dialog_run(GTK_DIALOG(dialog));

	gtk_widget_destroy(dialog);
}

/**
* Fonction appelée lors du clic du bouton Se connecter
* @usefulfor interface graphique
*/
static void clique_connect_serveur(GtkWidget * b)
{
	/*
	 * TO DO
	 */

}

/**
* Fonction desactivant bouton demarrer partie
* @usefulfor interface graphique
*/
void disable_button_start(void)
{
	gtk_widget_set_sensitive((GtkWidget *)
							 gtk_builder_get_object(p_builder, "button_start"),
							 FALSE);
}

// Ajout

/**
* Fonction appelee lors du clic du bouton Demarrer partie
* @usefulfor interface graphique
*/
static void clique_connect_adversaire(GtkWidget * b)
{
	if (newsockfd == -1)
	{
		// Desactivation bouton demarrer partie
		gtk_widget_set_sensitive((GtkWidget *)
								 gtk_builder_get_object(p_builder,
														"button_start"), FALSE);

		// Recuperation  adresse et port adversaire au format chaines caracteres
		addr_j2 = lecture_addr_adversaire();
		port_j2 = lecture_port_adversaire();

		printf("[Port joueur : %d] Adresse j2 lue : %s\n", port, addr_j2);
		printf("[Port joueur : %d] Port j2 lu : %s\n", port, port_j2);

		//utilise le signal SIGUSR1 pour lancer la socket
		pthread_kill(thr_id, SIGUSR1);
	}
}

// Fin ajout

/**
* Fonction desactivant les cases du damier
* pour attendre que l'utilisateur adverse joue (ignore les clics du joueurs courant)
* @usefulfor interface graphique
* @see #degele_damier(void)
*/
void gele_damier(void)
{
	gtk_widget_set_sensitive((GtkWidget *)
							 gtk_builder_get_object(p_builder, "eventboxA1"),
							 FALSE);
	gtk_widget_set_sensitive((GtkWidget *)
							 gtk_builder_get_object(p_builder, "eventboxB1"),
							 FALSE);
	gtk_widget_set_sensitive((GtkWidget *)
							 gtk_builder_get_object(p_builder, "eventboxC1"),
							 FALSE);
	gtk_widget_set_sensitive((GtkWidget *)
							 gtk_builder_get_object(p_builder, "eventboxD1"),
							 FALSE);
	gtk_widget_set_sensitive((GtkWidget *)
							 gtk_builder_get_object(p_builder, "eventboxE1"),
							 FALSE);
	gtk_widget_set_sensitive((GtkWidget *)
							 gtk_builder_get_object(p_builder, "eventboxF1"),
							 FALSE);
	gtk_widget_set_sensitive((GtkWidget *)
							 gtk_builder_get_object(p_builder, "eventboxG1"),
							 FALSE);
	gtk_widget_set_sensitive((GtkWidget *)
							 gtk_builder_get_object(p_builder, "eventboxH1"),
							 FALSE);
	gtk_widget_set_sensitive((GtkWidget *)
							 gtk_builder_get_object(p_builder, "eventboxA2"),
							 FALSE);
	gtk_widget_set_sensitive((GtkWidget *)
							 gtk_builder_get_object(p_builder, "eventboxB2"),
							 FALSE);
	gtk_widget_set_sensitive((GtkWidget *)
							 gtk_builder_get_object(p_builder, "eventboxC2"),
							 FALSE);
	gtk_widget_set_sensitive((GtkWidget *)
							 gtk_builder_get_object(p_builder, "eventboxD2"),
							 FALSE);
	gtk_widget_set_sensitive((GtkWidget *)
							 gtk_builder_get_object(p_builder, "eventboxE2"),
							 FALSE);
	gtk_widget_set_sensitive((GtkWidget *)
							 gtk_builder_get_object(p_builder, "eventboxF2"),
							 FALSE);
	gtk_widget_set_sensitive((GtkWidget *)
							 gtk_builder_get_object(p_builder, "eventboxG2"),
							 FALSE);
	gtk_widget_set_sensitive((GtkWidget *)
							 gtk_builder_get_object(p_builder, "eventboxH2"),
							 FALSE);
	gtk_widget_set_sensitive((GtkWidget *)
							 gtk_builder_get_object(p_builder, "eventboxA3"),
							 FALSE);
	gtk_widget_set_sensitive((GtkWidget *)
							 gtk_builder_get_object(p_builder, "eventboxB3"),
							 FALSE);
	gtk_widget_set_sensitive((GtkWidget *)
							 gtk_builder_get_object(p_builder, "eventboxC3"),
							 FALSE);
	gtk_widget_set_sensitive((GtkWidget *)
							 gtk_builder_get_object(p_builder, "eventboxD3"),
							 FALSE);
	gtk_widget_set_sensitive((GtkWidget *)
							 gtk_builder_get_object(p_builder, "eventboxE3"),
							 FALSE);
	gtk_widget_set_sensitive((GtkWidget *)
							 gtk_builder_get_object(p_builder, "eventboxF3"),
							 FALSE);
	gtk_widget_set_sensitive((GtkWidget *)
							 gtk_builder_get_object(p_builder, "eventboxG3"),
							 FALSE);
	gtk_widget_set_sensitive((GtkWidget *)
							 gtk_builder_get_object(p_builder, "eventboxH3"),
							 FALSE);
	gtk_widget_set_sensitive((GtkWidget *)
							 gtk_builder_get_object(p_builder, "eventboxA4"),
							 FALSE);
	gtk_widget_set_sensitive((GtkWidget *)
							 gtk_builder_get_object(p_builder, "eventboxB4"),
							 FALSE);
	gtk_widget_set_sensitive((GtkWidget *)
							 gtk_builder_get_object(p_builder, "eventboxC4"),
							 FALSE);
	gtk_widget_set_sensitive((GtkWidget *)
							 gtk_builder_get_object(p_builder, "eventboxD4"),
							 FALSE);
	gtk_widget_set_sensitive((GtkWidget *)
							 gtk_builder_get_object(p_builder, "eventboxE4"),
							 FALSE);
	gtk_widget_set_sensitive((GtkWidget *)
							 gtk_builder_get_object(p_builder, "eventboxF4"),
							 FALSE);
	gtk_widget_set_sensitive((GtkWidget *)
							 gtk_builder_get_object(p_builder, "eventboxG4"),
							 FALSE);
	gtk_widget_set_sensitive((GtkWidget *)
							 gtk_builder_get_object(p_builder, "eventboxH4"),
							 FALSE);
	gtk_widget_set_sensitive((GtkWidget *)
							 gtk_builder_get_object(p_builder, "eventboxA5"),
							 FALSE);
	gtk_widget_set_sensitive((GtkWidget *)
							 gtk_builder_get_object(p_builder, "eventboxB5"),
							 FALSE);
	gtk_widget_set_sensitive((GtkWidget *)
							 gtk_builder_get_object(p_builder, "eventboxC5"),
							 FALSE);
	gtk_widget_set_sensitive((GtkWidget *)
							 gtk_builder_get_object(p_builder, "eventboxD5"),
							 FALSE);
	gtk_widget_set_sensitive((GtkWidget *)
							 gtk_builder_get_object(p_builder, "eventboxE5"),
							 FALSE);
	gtk_widget_set_sensitive((GtkWidget *)
							 gtk_builder_get_object(p_builder, "eventboxF5"),
							 FALSE);
	gtk_widget_set_sensitive((GtkWidget *)
							 gtk_builder_get_object(p_builder, "eventboxG5"),
							 FALSE);
	gtk_widget_set_sensitive((GtkWidget *)
							 gtk_builder_get_object(p_builder, "eventboxH5"),
							 FALSE);
	gtk_widget_set_sensitive((GtkWidget *)
							 gtk_builder_get_object(p_builder, "eventboxA6"),
							 FALSE);
	gtk_widget_set_sensitive((GtkWidget *)
							 gtk_builder_get_object(p_builder, "eventboxB6"),
							 FALSE);
	gtk_widget_set_sensitive((GtkWidget *)
							 gtk_builder_get_object(p_builder, "eventboxC6"),
							 FALSE);
	gtk_widget_set_sensitive((GtkWidget *)
							 gtk_builder_get_object(p_builder, "eventboxD6"),
							 FALSE);
	gtk_widget_set_sensitive((GtkWidget *)
							 gtk_builder_get_object(p_builder, "eventboxE6"),
							 FALSE);
	gtk_widget_set_sensitive((GtkWidget *)
							 gtk_builder_get_object(p_builder, "eventboxF6"),
							 FALSE);
	gtk_widget_set_sensitive((GtkWidget *)
							 gtk_builder_get_object(p_builder, "eventboxG6"),
							 FALSE);
	gtk_widget_set_sensitive((GtkWidget *)
							 gtk_builder_get_object(p_builder, "eventboxH6"),
							 FALSE);
	gtk_widget_set_sensitive((GtkWidget *)
							 gtk_builder_get_object(p_builder, "eventboxA7"),
							 FALSE);
	gtk_widget_set_sensitive((GtkWidget *)
							 gtk_builder_get_object(p_builder, "eventboxB7"),
							 FALSE);
	gtk_widget_set_sensitive((GtkWidget *)
							 gtk_builder_get_object(p_builder, "eventboxC7"),
							 FALSE);
	gtk_widget_set_sensitive((GtkWidget *)
							 gtk_builder_get_object(p_builder, "eventboxD7"),
							 FALSE);
	gtk_widget_set_sensitive((GtkWidget *)
							 gtk_builder_get_object(p_builder, "eventboxE7"),
							 FALSE);
	gtk_widget_set_sensitive((GtkWidget *)
							 gtk_builder_get_object(p_builder, "eventboxF7"),
							 FALSE);
	gtk_widget_set_sensitive((GtkWidget *)
							 gtk_builder_get_object(p_builder, "eventboxG7"),
							 FALSE);
	gtk_widget_set_sensitive((GtkWidget *)
							 gtk_builder_get_object(p_builder, "eventboxH7"),
							 FALSE);
	gtk_widget_set_sensitive((GtkWidget *)
							 gtk_builder_get_object(p_builder, "eventboxA8"),
							 FALSE);
	gtk_widget_set_sensitive((GtkWidget *)
							 gtk_builder_get_object(p_builder, "eventboxB8"),
							 FALSE);
	gtk_widget_set_sensitive((GtkWidget *)
							 gtk_builder_get_object(p_builder, "eventboxC8"),
							 FALSE);
	gtk_widget_set_sensitive((GtkWidget *)
							 gtk_builder_get_object(p_builder, "eventboxD8"),
							 FALSE);
	gtk_widget_set_sensitive((GtkWidget *)
							 gtk_builder_get_object(p_builder, "eventboxE8"),
							 FALSE);
	gtk_widget_set_sensitive((GtkWidget *)
							 gtk_builder_get_object(p_builder, "eventboxF8"),
							 FALSE);
	gtk_widget_set_sensitive((GtkWidget *)
							 gtk_builder_get_object(p_builder, "eventboxG8"),
							 FALSE);
	gtk_widget_set_sensitive((GtkWidget *)
							 gtk_builder_get_object(p_builder, "eventboxH8"),
							 FALSE);
}

/**
* Fonction activant les cases du damier
* pour que l'utilisateur puisse jouer
* @usefulfor interface graphique
* @see #gele_damier(void)
*/
void degele_damier(void)
{
	gtk_widget_set_sensitive((GtkWidget *)
							 gtk_builder_get_object(p_builder, "eventboxA1"),
							 TRUE);
	gtk_widget_set_sensitive((GtkWidget *)
							 gtk_builder_get_object(p_builder, "eventboxB1"),
							 TRUE);
	gtk_widget_set_sensitive((GtkWidget *)
							 gtk_builder_get_object(p_builder, "eventboxC1"),
							 TRUE);
	gtk_widget_set_sensitive((GtkWidget *)
							 gtk_builder_get_object(p_builder, "eventboxD1"),
							 TRUE);
	gtk_widget_set_sensitive((GtkWidget *)
							 gtk_builder_get_object(p_builder, "eventboxE1"),
							 TRUE);
	gtk_widget_set_sensitive((GtkWidget *)
							 gtk_builder_get_object(p_builder, "eventboxF1"),
							 TRUE);
	gtk_widget_set_sensitive((GtkWidget *)
							 gtk_builder_get_object(p_builder, "eventboxG1"),
							 TRUE);
	gtk_widget_set_sensitive((GtkWidget *)
							 gtk_builder_get_object(p_builder, "eventboxH1"),
							 TRUE);
	gtk_widget_set_sensitive((GtkWidget *)
							 gtk_builder_get_object(p_builder, "eventboxA2"),
							 TRUE);
	gtk_widget_set_sensitive((GtkWidget *)
							 gtk_builder_get_object(p_builder, "eventboxB2"),
							 TRUE);
	gtk_widget_set_sensitive((GtkWidget *)
							 gtk_builder_get_object(p_builder, "eventboxC2"),
							 TRUE);
	gtk_widget_set_sensitive((GtkWidget *)
							 gtk_builder_get_object(p_builder, "eventboxD2"),
							 TRUE);
	gtk_widget_set_sensitive((GtkWidget *)
							 gtk_builder_get_object(p_builder, "eventboxE2"),
							 TRUE);
	gtk_widget_set_sensitive((GtkWidget *)
							 gtk_builder_get_object(p_builder, "eventboxF2"),
							 TRUE);
	gtk_widget_set_sensitive((GtkWidget *)
							 gtk_builder_get_object(p_builder, "eventboxG2"),
							 TRUE);
	gtk_widget_set_sensitive((GtkWidget *)
							 gtk_builder_get_object(p_builder, "eventboxH2"),
							 TRUE);
	gtk_widget_set_sensitive((GtkWidget *)
							 gtk_builder_get_object(p_builder, "eventboxA3"),
							 TRUE);
	gtk_widget_set_sensitive((GtkWidget *)
							 gtk_builder_get_object(p_builder, "eventboxB3"),
							 TRUE);
	gtk_widget_set_sensitive((GtkWidget *)
							 gtk_builder_get_object(p_builder, "eventboxC3"),
							 TRUE);
	gtk_widget_set_sensitive((GtkWidget *)
							 gtk_builder_get_object(p_builder, "eventboxD3"),
							 TRUE);
	gtk_widget_set_sensitive((GtkWidget *)
							 gtk_builder_get_object(p_builder, "eventboxE3"),
							 TRUE);
	gtk_widget_set_sensitive((GtkWidget *)
							 gtk_builder_get_object(p_builder, "eventboxF3"),
							 TRUE);
	gtk_widget_set_sensitive((GtkWidget *)
							 gtk_builder_get_object(p_builder, "eventboxG3"),
							 TRUE);
	gtk_widget_set_sensitive((GtkWidget *)
							 gtk_builder_get_object(p_builder, "eventboxH3"),
							 TRUE);
	gtk_widget_set_sensitive((GtkWidget *)
							 gtk_builder_get_object(p_builder, "eventboxA4"),
							 TRUE);
	gtk_widget_set_sensitive((GtkWidget *)
							 gtk_builder_get_object(p_builder, "eventboxB4"),
							 TRUE);
	gtk_widget_set_sensitive((GtkWidget *)
							 gtk_builder_get_object(p_builder, "eventboxC4"),
							 TRUE);
	gtk_widget_set_sensitive((GtkWidget *)
							 gtk_builder_get_object(p_builder, "eventboxD4"),
							 TRUE);
	gtk_widget_set_sensitive((GtkWidget *)
							 gtk_builder_get_object(p_builder, "eventboxE4"),
							 TRUE);
	gtk_widget_set_sensitive((GtkWidget *)
							 gtk_builder_get_object(p_builder, "eventboxF4"),
							 TRUE);
	gtk_widget_set_sensitive((GtkWidget *)
							 gtk_builder_get_object(p_builder, "eventboxG4"),
							 TRUE);
	gtk_widget_set_sensitive((GtkWidget *)
							 gtk_builder_get_object(p_builder, "eventboxH4"),
							 TRUE);
	gtk_widget_set_sensitive((GtkWidget *)
							 gtk_builder_get_object(p_builder, "eventboxA5"),
							 TRUE);
	gtk_widget_set_sensitive((GtkWidget *)
							 gtk_builder_get_object(p_builder, "eventboxB5"),
							 TRUE);
	gtk_widget_set_sensitive((GtkWidget *)
							 gtk_builder_get_object(p_builder, "eventboxC5"),
							 TRUE);
	gtk_widget_set_sensitive((GtkWidget *)
							 gtk_builder_get_object(p_builder, "eventboxD5"),
							 TRUE);
	gtk_widget_set_sensitive((GtkWidget *)
							 gtk_builder_get_object(p_builder, "eventboxE5"),
							 TRUE);
	gtk_widget_set_sensitive((GtkWidget *)
							 gtk_builder_get_object(p_builder, "eventboxF5"),
							 TRUE);
	gtk_widget_set_sensitive((GtkWidget *)
							 gtk_builder_get_object(p_builder, "eventboxG5"),
							 TRUE);
	gtk_widget_set_sensitive((GtkWidget *)
							 gtk_builder_get_object(p_builder, "eventboxH5"),
							 TRUE);
	gtk_widget_set_sensitive((GtkWidget *)
							 gtk_builder_get_object(p_builder, "eventboxA6"),
							 TRUE);
	gtk_widget_set_sensitive((GtkWidget *)
							 gtk_builder_get_object(p_builder, "eventboxB6"),
							 TRUE);
	gtk_widget_set_sensitive((GtkWidget *)
							 gtk_builder_get_object(p_builder, "eventboxC6"),
							 TRUE);
	gtk_widget_set_sensitive((GtkWidget *)
							 gtk_builder_get_object(p_builder, "eventboxD6"),
							 TRUE);
	gtk_widget_set_sensitive((GtkWidget *)
							 gtk_builder_get_object(p_builder, "eventboxE6"),
							 TRUE);
	gtk_widget_set_sensitive((GtkWidget *)
							 gtk_builder_get_object(p_builder, "eventboxF6"),
							 TRUE);
	gtk_widget_set_sensitive((GtkWidget *)
							 gtk_builder_get_object(p_builder, "eventboxG6"),
							 TRUE);
	gtk_widget_set_sensitive((GtkWidget *)
							 gtk_builder_get_object(p_builder, "eventboxH6"),
							 TRUE);
	gtk_widget_set_sensitive((GtkWidget *)
							 gtk_builder_get_object(p_builder, "eventboxA7"),
							 TRUE);
	gtk_widget_set_sensitive((GtkWidget *)
							 gtk_builder_get_object(p_builder, "eventboxB7"),
							 TRUE);
	gtk_widget_set_sensitive((GtkWidget *)
							 gtk_builder_get_object(p_builder, "eventboxC7"),
							 TRUE);
	gtk_widget_set_sensitive((GtkWidget *)
							 gtk_builder_get_object(p_builder, "eventboxD7"),
							 TRUE);
	gtk_widget_set_sensitive((GtkWidget *)
							 gtk_builder_get_object(p_builder, "eventboxE7"),
							 TRUE);
	gtk_widget_set_sensitive((GtkWidget *)
							 gtk_builder_get_object(p_builder, "eventboxF7"),
							 TRUE);
	gtk_widget_set_sensitive((GtkWidget *)
							 gtk_builder_get_object(p_builder, "eventboxG7"),
							 TRUE);
	gtk_widget_set_sensitive((GtkWidget *)
							 gtk_builder_get_object(p_builder, "eventboxH7"),
							 TRUE);
	gtk_widget_set_sensitive((GtkWidget *)
							 gtk_builder_get_object(p_builder, "eventboxA8"),
							 TRUE);
	gtk_widget_set_sensitive((GtkWidget *)
							 gtk_builder_get_object(p_builder, "eventboxB8"),
							 TRUE);
	gtk_widget_set_sensitive((GtkWidget *)
							 gtk_builder_get_object(p_builder, "eventboxC8"),
							 TRUE);
	gtk_widget_set_sensitive((GtkWidget *)
							 gtk_builder_get_object(p_builder, "eventboxD8"),
							 TRUE);
	gtk_widget_set_sensitive((GtkWidget *)
							 gtk_builder_get_object(p_builder, "eventboxE8"),
							 TRUE);
	gtk_widget_set_sensitive((GtkWidget *)
							 gtk_builder_get_object(p_builder, "eventboxF8"),
							 TRUE);
	gtk_widget_set_sensitive((GtkWidget *)
							 gtk_builder_get_object(p_builder, "eventboxG8"),
							 TRUE);
	gtk_widget_set_sensitive((GtkWidget *)
							 gtk_builder_get_object(p_builder, "eventboxH8"),
							 TRUE);
}

/**
* Fonction permettant d'initialiser le plateau de jeu
* @usefulfor interface graphique
*/
void init_interface_jeu(void)
{

	//initialisation score à 0
	set_score_J1(0);
	set_score_J2(0);
/**
* Initilisation du damier (D4=blanc, E4=noir, D5=noir, E5=blanc)
* D2 blanc équivaut à {3, 1, 1}
* (car D, 4e lettre, donc => 3)
* (4, 4e chiffre, donc => 1)
*/
	set_case(3, 3, BLANC);

	set_case(4, 3, NOIR);

	set_case(3, 4, NOIR);

	set_case(4, 4, BLANC);

/**
* Initialisation des scores et des joueurs
*/
	if (get_couleur() == BLANC)
	{
		set_label_J1("Vous");
		set_label_J2("Adversaire");
	}
	else
	{							//noir
		set_label_J1("Adversaire");
		set_label_J2("Vous");
	}

	/*
	 * TO DO
	 */

}

/**
* Fonction reinitialisant la liste des joueurs sur l'interface graphique
* @usefulfor interface graphique
*/
void reset_liste_joueurs(void)
{
	GtkTextIter start, end;

	gtk_text_buffer_get_start_iter(GTK_TEXT_BUFFER
								   (gtk_text_view_get_buffer
									(GTK_TEXT_VIEW
									 (gtk_builder_get_object
									  (p_builder, "textview_joueurs")))),
								   &start);
	gtk_text_buffer_get_end_iter(GTK_TEXT_BUFFER
								 (gtk_text_view_get_buffer
								  (GTK_TEXT_VIEW
								   (gtk_builder_get_object
									(p_builder, "textview_joueurs")))), &end);

	gtk_text_buffer_delete(GTK_TEXT_BUFFER
						   (gtk_text_view_get_buffer
							(GTK_TEXT_VIEW
							 (gtk_builder_get_object
							  (p_builder, "textview_joueurs")))), &start, &end);
}

/**
* Fonction permettant d'ajouter un joueur
* dans la liste des joueurs sur l'interface graphique
* @param login le login du joueur
* @param adresse l'adresse ip du joueur
* @param port le port du joueur
* @usefulfor interface graphique
*/
void affich_joueur(char *login, char *adresse, char *port)
{
	const gchar *joueur;

	joueur = g_strconcat(login, " - ", adresse, " : ", port, "\n", NULL);

	gtk_text_buffer_insert_at_cursor(GTK_TEXT_BUFFER
									 (gtk_text_view_get_buffer
									  (GTK_TEXT_VIEW
									   (gtk_builder_get_object
										(p_builder, "textview_joueurs")))),
									 joueur, strlen(joueur));
}

/**

* Fonction exécutée par le thread gérant
* les communications à travers la socket
*/
static void *f_com_socket(void *p_arg)
{

	int i, nbytes, col, lig;

	char buf[MAXDATASIZE], *tmp, *p_parse;
	int len, bytes_sent, t_msg_recu;

	sigset_t signal_mask;
	int fd_signal, rv;

	uint16_t type_msg, col_j2;
	uint16_t ucol, ulig;

	char *token;
	char *saveptr;

	//ajout initiasation requete
	struct requete req;

	/* Association descripteur au signal SIGUSR1 */
	sigemptyset(&signal_mask);
	sigaddset(&signal_mask, SIGUSR1);

	if (sigprocmask(SIG_BLOCK, &signal_mask, NULL) == -1)
	{
		printf("[Port joueur %d] Erreur sigprocmask\n", port);
		return 0;
	}

	fd_signal = signalfd(-1, &signal_mask, 0);

	if (fd_signal == -1)
	{
		printf("[Port joueur %d] Erreur signalfd\n", port);
		return 0;
	}

	/*
	 * Ajout descripteur du signal dans ensemble
	 * de descripteur utilisé avec fonction select
	 */
	FD_SET(fd_signal, &master);

	if (fd_signal > fdmax)
	{
		fdmax = fd_signal;
	}


	while (1)
	{
		read_fds = master;		// copie des ensembles

		if (select(fdmax + 1, &read_fds, NULL, NULL, NULL) == -1)
		{
			perror("Problème avec select");
			exit(4);
		}

		printf("[Port joueur %d] Entrée dans boucle for\n", port);
		for (i = 0; i <= fdmax; ++i)
		{
			/*printf
			   ("[Port joueur %d] newsockfd=%d, iteration %d boucle for\n",
			   port, newsockfd, i); */

			if (FD_ISSET(i, &read_fds))
			{
				if (i == fd_signal)
				{
					printf("i = fd_signal\n");
					/*
					 * Cas où de l'envoie du signal par l'interface graphique
					 * pour connexion au joueur adverse
					 */
					if (newsockfd == -1)
					{
						memset(&hints, 0, sizeof(hints));	//rempli hints de 0
						hints.ai_family = AF_UNSPEC;	//ça peut donc être une IPV4 ou IPV6 ; ça gère les 2
						hints.ai_socktype = SOCK_STREAM;	// communication connectée ; Utilise le protocole TCP

						rv = getaddrinfo(addr_j2, port_j2, &hints, &servinfo);	// rempli automatiquement une structure addinfo (param4)

						if (rv != 0)	// getaddrinfo a-t-il fonctionné ? 0 = Non
						{
							fprintf(stderr, "getaddrinfo:%s\n",
									gai_strerror(rv));
						}
						//création socket et attachement
						for (p = servinfo; p != NULL; p = p->ai_next)
						{
							if ((newsockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1)	//sockfd est la socket du client
							{
								perror("client: socket");
								continue;
							}
							if (connect(newsockfd, p->ai_addr, p->ai_addrlen) == -1)	//attachement de la socket - client
							{
								close(newsockfd);
								//FD_CLR(newsockfd, &master);
								perror("client: connect");
								continue;
							}
							break;
						}
						if (p == NULL)
						{
							fprintf(stderr, "server : failed to bind\n");
							return 2;
						}

						freeaddrinfo(servinfo);	//libère la structure

						/*
						 * Ajout descripteur du signal dans ensemble
						 * de descripteur utilisé avec fonction select
						 */
						FD_SET(newsockfd, &master);

						if (newsockfd > fdmax)
						{
							fdmax = newsockfd;
						}

						close(sockfd);
						FD_CLR(sockfd, &master);

						//fermeture et suppression de fd_signal de l'ensemble master
						close(fd_signal);
						FD_CLR(fd_signal, &master);

						//Choix de couleur à compléter

						set_couleur(NOIR);
						req.type = htons((uint16_t) TYPE_COULEUR);
						req.couleur =
							htons((uint16_t) get_couleur_adversaire());

						bzero(buf, MAXDATASIZE);

						printf("Requête à envoyer %u : %u\n", req.type,
							   req.couleur);
						snprintf(buf, MAXDATASIZE, "%u, %u", req.type,
								 req.couleur);

						printf("Envoyer requête %s\n", buf);
						if (send(newsockfd, &buf, strlen(buf), 0) == -1)
						{
							perror("send");
							close(newsockfd);
						}

						//initialisation interface graphique
						init_interface_jeu();
					}
				}

				if (i == sockfd)
				{				//acceptation connexion adversaire

					printf("i == sockfd\n");

					if (newsockfd == -1)
					{
						addr_size = sizeof(their_addr);
						/*
						 * Accepte une connexion client
						 * et retourne nouveau descripteur de socket de service
						 * pour traiter la requête du client
						 * Fonction bloquante
						 * @param1 socket d'écoute côté serveur
						 * @param2 structure adresse + port du client
						 * @param3 taille du param2 (cf ci-dessus)
						 */
						newsockfd =
							accept(sockfd, their_addr,
								   (socklen_t *) & addr_size);


						if (newsockfd == -1)
						{
							perror("problème avec accept");
						}
						else
						{
							/*
							 * Ajout du nouveau descripteur dans ensemble
							 * de descripteur utilisé avec fonction select
							 */
							FD_SET(newsockfd, &master);

							if (newsockfd > fdmax)
							{
								fdmax = newsockfd;
							}

							//


							//fermeture socket écoute car connexion déjà établie avec adversaire
							printf
								("[Port joueur : %d] Bloc if du serveur, fermeture socket écoute\n",
								 port);
							FD_CLR(sockfd, &master);
							close(sockfd);
						}

						//fermeture et suppression de FD_SIGNAL de l'ensemble master
						close(fd_signal);
						FD_CLR(fd_signal, &master);

						// Réception et traitement des messages du joueur adverse
						bzero(buf, MAXDATASIZE);
						recv(newsockfd, buf, MAXDATASIZE, 0);

						token = strtok_r(buf, ",", &saveptr);
						sscanf(token, "%u", &(req.type));
						req.type = ntohs(req.type);
						printf("type message %u\n", req.type);

						if (req.type == 0)
						{
							token = strtok_r(NULL, ",", &saveptr);
							sscanf(token, "%u", &(req.couleur));
							req.couleur = ntohs(req.couleur);
							printf("couleur %u\n", req.couleur);
							set_couleur(req.couleur);
						}

						init_interface_jeu();

						gtk_widget_set_sensitive((GtkWidget *)
												 gtk_builder_get_object
												 (p_builder, "button_start"),
												 FALSE);
					}
				}
				else
				{
					printf("else\n");

					if (i == newsockfd)
					{
						printf("else suite\n");
						// Réception et traitement des messages du joueur adverse
						bzero(buf, MAXDATASIZE);
						nbytes = recv(newsockfd, buf, MAXDATASIZE, 0);

						printf("reception de %d octets reçus\n", nbytes);

						token = strtok_r(buf, ",", &saveptr);
						sscanf(token, "%u", &(req.type));
						req.type = ntohs(req.type);
						printf("type message %u\n", req.type);

						if (req.type == TYPE_COORD)
						{
							token = strtok_r(NULL, ",", &saveptr);
							sscanf(token, "%u", &(req.x));
							req.x = ntohs(req.x);
							printf("x %u\n", req.x);

							token = strtok_r(NULL, ",", &saveptr);
							sscanf(token, "%u", &(req.y));
							req.y = ntohs(req.y);
							printf("y %u\n", req.y);
							set_case(req.x, req.y, get_couleur_adversaire());
							changements_case(req.x, req.y,
											 get_couleur_adversaire());
							degele_damier();
						}
						else if (req.type == TYPE_IMPOSSIBILITE_COUP)
						{
							printf("adversaire n'a pas plus jouer\n");
							degele_damier();
						}
						else if (req.type == TYPE_FIN)
						{
							printf("fin du jeu\n");
							degele_damier();
						}
					}
				}
			}
		}
	}

	return (NULL);
}

// Fin ajout



/** INT MAIN */
int main(int argc, char **argv)
{
	int i, j, ret;

	if (argc != 2)
	{
		printf("\nPrototype : ./othello num_port\n\n");
		exit(1);
	}


	/* Initialisation de GTK+ */
	gtk_init(&argc, &argv);

	/* Création d'un nouveau GtkBuilder */
	p_builder = gtk_builder_new();

	if (p_builder != NULL)
	{
		/* Chargement du XML dans p_builder */
		gtk_builder_add_from_file(p_builder, "UI_Glade/Othello.glade", &p_err);

		if (p_err == NULL)
		{
			/* Récupération d'un pointeur sur la fenêtre. */
			GtkWidget *p_win =
				(GtkWidget *) gtk_builder_get_object(p_builder, "window1");

			/* En plus, ajout du port dans le titre */
			char titre[80];
			strcpy(titre, "Projet Othello - ");
			strcat(titre, argv[1]);
			gtk_window_set_title(GTK_WINDOW(p_win), titre);


			/* DÉBUT Gestion événement clic pour chacune des cases du damier */
			g_signal_connect(gtk_builder_get_object(p_builder, "eventboxA1"),
							 "button_press_event", G_CALLBACK(coup_joueur),
							 NULL);
			g_signal_connect(gtk_builder_get_object(p_builder, "eventboxB1"),
							 "button_press_event", G_CALLBACK(coup_joueur),
							 NULL);
			g_signal_connect(gtk_builder_get_object(p_builder, "eventboxC1"),
							 "button_press_event", G_CALLBACK(coup_joueur),
							 NULL);
			g_signal_connect(gtk_builder_get_object(p_builder, "eventboxD1"),
							 "button_press_event", G_CALLBACK(coup_joueur),
							 NULL);
			g_signal_connect(gtk_builder_get_object(p_builder, "eventboxE1"),
							 "button_press_event", G_CALLBACK(coup_joueur),
							 NULL);
			g_signal_connect(gtk_builder_get_object(p_builder, "eventboxF1"),
							 "button_press_event", G_CALLBACK(coup_joueur),
							 NULL);
			g_signal_connect(gtk_builder_get_object(p_builder, "eventboxG1"),
							 "button_press_event", G_CALLBACK(coup_joueur),
							 NULL);
			g_signal_connect(gtk_builder_get_object(p_builder, "eventboxH1"),
							 "button_press_event", G_CALLBACK(coup_joueur),
							 NULL);
			g_signal_connect(gtk_builder_get_object(p_builder, "eventboxA2"),
							 "button_press_event", G_CALLBACK(coup_joueur),
							 NULL);
			g_signal_connect(gtk_builder_get_object(p_builder, "eventboxB2"),
							 "button_press_event", G_CALLBACK(coup_joueur),
							 NULL);
			g_signal_connect(gtk_builder_get_object(p_builder, "eventboxC2"),
							 "button_press_event", G_CALLBACK(coup_joueur),
							 NULL);
			g_signal_connect(gtk_builder_get_object(p_builder, "eventboxD2"),
							 "button_press_event", G_CALLBACK(coup_joueur),
							 NULL);
			g_signal_connect(gtk_builder_get_object(p_builder, "eventboxE2"),
							 "button_press_event", G_CALLBACK(coup_joueur),
							 NULL);
			g_signal_connect(gtk_builder_get_object(p_builder, "eventboxF2"),
							 "button_press_event", G_CALLBACK(coup_joueur),
							 NULL);
			g_signal_connect(gtk_builder_get_object(p_builder, "eventboxG2"),
							 "button_press_event", G_CALLBACK(coup_joueur),
							 NULL);
			g_signal_connect(gtk_builder_get_object(p_builder, "eventboxH2"),
							 "button_press_event", G_CALLBACK(coup_joueur),
							 NULL);
			g_signal_connect(gtk_builder_get_object(p_builder, "eventboxA3"),
							 "button_press_event", G_CALLBACK(coup_joueur),
							 NULL);
			g_signal_connect(gtk_builder_get_object(p_builder, "eventboxB3"),
							 "button_press_event", G_CALLBACK(coup_joueur),
							 NULL);
			g_signal_connect(gtk_builder_get_object(p_builder, "eventboxC3"),
							 "button_press_event", G_CALLBACK(coup_joueur),
							 NULL);
			g_signal_connect(gtk_builder_get_object(p_builder, "eventboxD3"),
							 "button_press_event", G_CALLBACK(coup_joueur),
							 NULL);
			g_signal_connect(gtk_builder_get_object(p_builder, "eventboxE3"),
							 "button_press_event", G_CALLBACK(coup_joueur),
							 NULL);
			g_signal_connect(gtk_builder_get_object(p_builder, "eventboxF3"),
							 "button_press_event", G_CALLBACK(coup_joueur),
							 NULL);
			g_signal_connect(gtk_builder_get_object(p_builder, "eventboxG3"),
							 "button_press_event", G_CALLBACK(coup_joueur),
							 NULL);
			g_signal_connect(gtk_builder_get_object(p_builder, "eventboxH3"),
							 "button_press_event", G_CALLBACK(coup_joueur),
							 NULL);
			g_signal_connect(gtk_builder_get_object(p_builder, "eventboxA4"),
							 "button_press_event", G_CALLBACK(coup_joueur),
							 NULL);
			g_signal_connect(gtk_builder_get_object(p_builder, "eventboxB4"),
							 "button_press_event", G_CALLBACK(coup_joueur),
							 NULL);
			g_signal_connect(gtk_builder_get_object(p_builder, "eventboxC4"),
							 "button_press_event", G_CALLBACK(coup_joueur),
							 NULL);
			g_signal_connect(gtk_builder_get_object(p_builder, "eventboxD4"),
							 "button_press_event", G_CALLBACK(coup_joueur),
							 NULL);
			g_signal_connect(gtk_builder_get_object(p_builder, "eventboxE4"),
							 "button_press_event", G_CALLBACK(coup_joueur),
							 NULL);
			g_signal_connect(gtk_builder_get_object(p_builder, "eventboxF4"),
							 "button_press_event", G_CALLBACK(coup_joueur),
							 NULL);
			g_signal_connect(gtk_builder_get_object(p_builder, "eventboxG4"),
							 "button_press_event", G_CALLBACK(coup_joueur),
							 NULL);
			g_signal_connect(gtk_builder_get_object(p_builder, "eventboxH4"),
							 "button_press_event", G_CALLBACK(coup_joueur),
							 NULL);
			g_signal_connect(gtk_builder_get_object(p_builder, "eventboxA5"),
							 "button_press_event", G_CALLBACK(coup_joueur),
							 NULL);
			g_signal_connect(gtk_builder_get_object(p_builder, "eventboxB5"),
							 "button_press_event", G_CALLBACK(coup_joueur),
							 NULL);
			g_signal_connect(gtk_builder_get_object(p_builder, "eventboxC5"),
							 "button_press_event", G_CALLBACK(coup_joueur),
							 NULL);
			g_signal_connect(gtk_builder_get_object(p_builder, "eventboxD5"),
							 "button_press_event", G_CALLBACK(coup_joueur),
							 NULL);
			g_signal_connect(gtk_builder_get_object(p_builder, "eventboxE5"),
							 "button_press_event", G_CALLBACK(coup_joueur),
							 NULL);
			g_signal_connect(gtk_builder_get_object(p_builder, "eventboxF5"),
							 "button_press_event", G_CALLBACK(coup_joueur),
							 NULL);
			g_signal_connect(gtk_builder_get_object(p_builder, "eventboxG5"),
							 "button_press_event", G_CALLBACK(coup_joueur),
							 NULL);
			g_signal_connect(gtk_builder_get_object(p_builder, "eventboxH5"),
							 "button_press_event", G_CALLBACK(coup_joueur),
							 NULL);
			g_signal_connect(gtk_builder_get_object(p_builder, "eventboxA6"),
							 "button_press_event", G_CALLBACK(coup_joueur),
							 NULL);
			g_signal_connect(gtk_builder_get_object(p_builder, "eventboxB6"),
							 "button_press_event", G_CALLBACK(coup_joueur),
							 NULL);
			g_signal_connect(gtk_builder_get_object(p_builder, "eventboxC6"),
							 "button_press_event", G_CALLBACK(coup_joueur),
							 NULL);
			g_signal_connect(gtk_builder_get_object(p_builder, "eventboxD6"),
							 "button_press_event", G_CALLBACK(coup_joueur),
							 NULL);
			g_signal_connect(gtk_builder_get_object(p_builder, "eventboxE6"),
							 "button_press_event", G_CALLBACK(coup_joueur),
							 NULL);
			g_signal_connect(gtk_builder_get_object(p_builder, "eventboxF6"),
							 "button_press_event", G_CALLBACK(coup_joueur),
							 NULL);
			g_signal_connect(gtk_builder_get_object(p_builder, "eventboxG6"),
							 "button_press_event", G_CALLBACK(coup_joueur),
							 NULL);
			g_signal_connect(gtk_builder_get_object(p_builder, "eventboxH6"),
							 "button_press_event", G_CALLBACK(coup_joueur),
							 NULL);
			g_signal_connect(gtk_builder_get_object(p_builder, "eventboxA7"),
							 "button_press_event", G_CALLBACK(coup_joueur),
							 NULL);
			g_signal_connect(gtk_builder_get_object(p_builder, "eventboxB7"),
							 "button_press_event", G_CALLBACK(coup_joueur),
							 NULL);
			g_signal_connect(gtk_builder_get_object(p_builder, "eventboxC7"),
							 "button_press_event", G_CALLBACK(coup_joueur),
							 NULL);
			g_signal_connect(gtk_builder_get_object(p_builder, "eventboxD7"),
							 "button_press_event", G_CALLBACK(coup_joueur),
							 NULL);
			g_signal_connect(gtk_builder_get_object(p_builder, "eventboxE7"),
							 "button_press_event", G_CALLBACK(coup_joueur),
							 NULL);
			g_signal_connect(gtk_builder_get_object(p_builder, "eventboxF7"),
							 "button_press_event", G_CALLBACK(coup_joueur),
							 NULL);
			g_signal_connect(gtk_builder_get_object(p_builder, "eventboxG7"),
							 "button_press_event", G_CALLBACK(coup_joueur),
							 NULL);
			g_signal_connect(gtk_builder_get_object(p_builder, "eventboxH7"),
							 "button_press_event", G_CALLBACK(coup_joueur),
							 NULL);
			g_signal_connect(gtk_builder_get_object(p_builder, "eventboxA8"),
							 "button_press_event", G_CALLBACK(coup_joueur),
							 NULL);
			g_signal_connect(gtk_builder_get_object(p_builder, "eventboxB8"),
							 "button_press_event", G_CALLBACK(coup_joueur),
							 NULL);
			g_signal_connect(gtk_builder_get_object(p_builder, "eventboxC8"),
							 "button_press_event", G_CALLBACK(coup_joueur),
							 NULL);
			g_signal_connect(gtk_builder_get_object(p_builder, "eventboxD8"),
							 "button_press_event", G_CALLBACK(coup_joueur),
							 NULL);
			g_signal_connect(gtk_builder_get_object(p_builder, "eventboxE8"),
							 "button_press_event", G_CALLBACK(coup_joueur),
							 NULL);
			g_signal_connect(gtk_builder_get_object(p_builder, "eventboxF8"),
							 "button_press_event", G_CALLBACK(coup_joueur),
							 NULL);
			g_signal_connect(gtk_builder_get_object(p_builder, "eventboxG8"),
							 "button_press_event", G_CALLBACK(coup_joueur),
							 NULL);
			g_signal_connect(gtk_builder_get_object(p_builder, "eventboxH8"),
							 "button_press_event", G_CALLBACK(coup_joueur),
							 NULL);
			/* FIN Gestion événement clic pour chacune des cases du damier */

			/* Gestion clic boutons interface */
			g_signal_connect(gtk_builder_get_object
							 (p_builder, "button_connect"), "clicked",
							 G_CALLBACK(clique_connect_serveur), NULL);
			g_signal_connect(gtk_builder_get_object(p_builder, "button_start"),
							 "clicked", G_CALLBACK(clique_connect_adversaire),
							 NULL);

			/* Gestion clic bouton fermeture fenêtre */
			g_signal_connect_swapped(G_OBJECT(p_win), "destroy",
									 G_CALLBACK(gtk_main_quit), NULL);



			/* Récuperation numéro port donné en paramètre */
			port = atoi(argv[1]);

			//gele_damier();

			/* Initialisation du damier de jeu ; toutes les cases valent -1 */
			for (i = 0; i < TAILLE_DAMIER; ++i)
			{
				for (j = 0; j < TAILLE_DAMIER; ++j)
				{
					damier[i][j] = VIDE;
				}
			}

			/*
			 * Initialisation socket et autres objets,
			 * et création thread pour communication avec joueur adverse
			 */

			/*
			 * Acceptation connexion adversaire
			 */
			memset(&hints, 0, sizeof(hints));	//idem que ci-dessus
			hints.ai_family = AF_UNSPEC;
			hints.ai_socktype = SOCK_STREAM;
			hints.ai_flags = AI_PASSIVE;	//trouve automatiquement mon IP
			// car premier param de getaddrinfo = NULL
			status = getaddrinfo(NULL, argv[1], &hints, &servinfo);
			if (status != 0)
			{
				fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
				return 1;
			}
			// Création socket et attachement
			for (p = servinfo; p != NULL; p = p->ai_next)
			{
				if ((sockfd =
					 socket(p->ai_family, p->ai_socktype,
							p->ai_protocol)) == -1)
				{
					perror("server: socket");
					continue;
				}
				if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1)	//attachement de la socket - serveur
				{
					close(sockfd);
					//FD_CLR(sockfd, &master);
					perror("server: bind");
					continue;
				}
				printf("[Port joueur : %s] Ok socket correct\n", argv[1]);
				break;
			}
			if (p == NULL)
			{
				fprintf(stderr, "server: failed to bind\n");
				return 2;
			}
			freeaddrinfo(servinfo);	// Libère structure
			/*
			 * le serveur ecoute et est en attente d'une demande de connexion
			 * socket d'écoute cote serveur
			 * Fonction bloquante
			 * @param2 est nombre max de connexion en attente de traitement
			 */
			listen(sockfd, 1);

			FD_ZERO(&master);
			FD_ZERO(&read_fds);

			/*
			 * Ajout descripteur du signal dans ensemble
			 * de descripteur utilisé avec fonction select
			 */
			FD_SET(sockfd, &master);
			read_fds = master;	//copie des ensembles
			fdmax = sockfd;

			/* Création thread serveur écoute message */
			ret = pthread_create(&thr_id, NULL, f_com_socket, NULL);
			if (ret != 0)
			{
				sprintf(stderr, "%s", strerror(ret));
				exit(1);
			}

			gtk_widget_show_all(p_win);
			gtk_main();
		}
		else
		{
			/* Affichage du message d'erreur de GTK+ */
			g_error("%s", p_err->message);
			g_error_free(p_err);
		}
	}


	return EXIT_SUCCESS;
}
