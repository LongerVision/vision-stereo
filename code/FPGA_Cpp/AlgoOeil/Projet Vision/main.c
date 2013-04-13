#include <time.h>
#include <math.h>
#include <stdio.h>    /* Ajouter ...\c6000\cgtools\lib\rts6701.lib  au projet */   
#include "c:\apps\ti\c6000\imglib\include\histogram.h"
#include "defs.h"

#define TAILLE_HISTOGRAMME 256
#define MAX_IMAGE_SIZE 320*251
// 120*120
//#define ID_ZONES

int nbr_pixels;

// pour l'identification de zones
  #define NBR_ZONES_MAX 50
  //int zones_x[NBR_ZONES_MAX];
  //int zones_y[NBR_ZONES_MAX];
  //float zones_taille_zone[NBR_ZONES_MAX];
  //int zones_surface_zone[NBR_ZONES_MAX];
  float zones_rapport[NBR_ZONES_MAX];
  int zones_x_inf[NBR_ZONES_MAX];
  int zones_x_sup[NBR_ZONES_MAX];
  int zones_y_inf[NBR_ZONES_MAX];
  int zones_y_sup[NBR_ZONES_MAX];
  //int zones[NBR_ZONES_MAX]; // num�ros des zones pour le tri

/* histogrammes de l'image (pour la recherche du seuil optimal de segmentation) */    
#pragma DATA_ALIGN(histogramme,8);
#pragma DATA_ALIGN(histogramme_cumule,8);
#pragma DATA_ALIGN(histogramme_filtre,8);
unsigned short histogramme[TAILLE_HISTOGRAMME];
unsigned short histogramme_cumule[TAILLE_HISTOGRAMME];
unsigned short histogramme_filtre[TAILLE_HISTOGRAMME];

//short int histogramme2_temp[TAILLE_HISTOGRAMME*4];
//short int histogramme2[TAILLE_HISTOGRAMME];

extern IMAGE_LOCATION unsigned char raster[];
extern short nbr_lignes;
extern short nbr_colonnes;      
extern short diametre_x;
extern short diametre_y;
extern short centre_x_db;
extern short centre_y_db;
extern short num_image;


unsigned char b_inf, b_sup;

int temp;

float centre_x;
float centre_y;

#ifdef CALCULER_IMAGE_SEUILLEE
  //#pragma DATA_ALIGN("image_seuil", 8);
  IMAGE_LOCATION unsigned char image_seuil[MAX_IMAGE_SIZE]; // buffer pour l'image seuill�e (taille max fix�e)
#endif
#define TAILLE_IMAGE_SOUS_ECH MAX_IMAGE_SIZE/(8*8)
unsigned char image_sous_ech[TAILLE_IMAGE_SOUS_ECH]; // buffer pour l'image sous-ech.
                                                     // taille de bloc de min 4x4
unsigned char image_sous_ech_marque[TAILLE_IMAGE_SOUS_ECH]; // marquage de l'image sous-�ch. (permet de voir si on est d�j� pass� par une case lors du parcours de zone




init_recherche_seuil_optimal() {
/* ------------------------------------------------
 * Initialisation de la recherche du seuil optimal
 * pour la segmentation de l'image.
 *
 * Les op�rations suivantes sont effectu�es :
 *     1) calcul de l'histogramme de l'image
 *     2) calcul de l'histogramme cumul�
 *     3) filtrage de l'histogramme (m�dian + passe-bas)
 *
 * Input :
 *   raster (contient les pixels de l'image)
 *   nbr_lignes
 *   nbr_colonnes
 * Output : 
 *   -
 * Modifies :
 *   histogramme[] : histogramme filtr�
 *   histogramme_cumule[] : histogramme cumul�
 *   histogramme_filtre[] : histogramme filtr� m�dian uniquement (var. tempopraire)
 */                      

/* Versions d'algorithmes (par ordre de vitesse croissante) :
     HISTO       : 1..3  (versions 2 et 3 pas identiques � la 1 pour les images 27 et 30)
     CUMUL_HISTO : 1..4
     PASSE_BAS   : 1..3
*/ 
 #define ALGO_VERSION_HISTO       3
 #define ALGO_VERSION_CUMUL_HISTO 4
 #define ALGO_VERSION_PASSE_BAS   3
 
 /* d�finition des donn�es pour les filtres */
 #define ORDRE_FILTRE_MEDIAN 5
 #define ORDRE_FILTRE_PASSE_BAS 3
 #define ORDRE_FILTRE_PASSE_BAS_UPPER_POW2 4
 #define ORDRE_FILTRE_MEDIAN_DIV2 ORDRE_FILTRE_MEDIAN / 2
 #define ORDRE_FILTRE_PASSE_BAS_DIV2 ORDRE_FILTRE_PASSE_BAS / 2
 
 /* variables locales */
 //unsigned int nbr_pixels_8; // nombre de pixels multiple de 8 

 // variables pour le calcul de l'histogramme
 #if   ALGO_VERSION_HISTO==3
   int n_histo; // compteur de boucles
   unsigned int word1;
   unsigned char i1_1, i1_2, i1_3, i1_4;
   unsigned int* raster_temp;
   unsigned int h1,h2,h3,h4;
 #elif ALGO_VERSION_HISTO==2
   int n_histo; // compteur de boucles
   unsigned int word1, word2;
   unsigned char i1_1, i1_2, i1_3, i1_4;
   unsigned char i2_1, i2_2, i2_3, i2_4;
   unsigned int* raster_temp;  // adresse des 8 pixels � transf�rer
   unsigned int h1,h2,h3,h4,h5,h6,h7,h8;
 #elif ALGO_VERSION_HISTO==1
   int n_histo; // compteur de boucles
   unsigned char pixel;  // couleur du pixel courant
 #endif


 // variables pour le calcul de l'histogramme cumul�
 #if   ALGO_VERSION_CUMUL_HISTO==4
   int n_cumul; // compteur de boucles
   unsigned short somme_cumulee; // nombre de pixels cumule
   unsigned short s0,s1,s2,s3,ss0;
 #elif ALGO_VERSION_CUMUL_HISTO==3
   int n_cumul; // compteur de boucles
   unsigned short somme_cumulee; // nombre de pixels cumule
   unsigned short s0,s1,s2,s3;
 #elif ALGO_VERSION_CUMUL_HISTO==2
   int n_cumul; // compteur de boucles
   unsigned short somme_cumulee; // nombre de pixels cumule
 #elif ALGO_VERSION_CUMUL_HISTO==1
   int n_cumul; // compteur de boucles
 #endif

 
 // variables pour le filtre m�dian
 #if ORDRE_FILTRE_MEDIAN==5
    // code sp�cial pour le filtre m�dian d'ordre 5 (plus efficace)
    int n_median; // compteur de boucles
    unsigned short r0,r1,r2,r3,r4,r5,r10; // pour le median
    unsigned short min01234, max; // pour le median
 #else
   unsigned char  k,m; /* compteur de boucles pour le median */
   unsigned short filtre_median[ORDRE_FILTRE_MEDIAN];
   unsigned short temp_median; // variable d'�change du filtre median
   unsigned int   index;  // pour le calcul du filtre median
 #endif


 // variables pour le filtre passe-bas
 #if   ALGO_VERSION_PASSE_BAS==3
   int n_passe_bas; // compteur de boucle
   unsigned short somme_passe_bas; // somme pour la moyenne
   unsigned int next_to_remove1; // pour le calcul du filtre passe-bas
   unsigned int next_to_remove2; // pour le calcul du filtre passe-bas
   unsigned int next_to_remove3; // pour le calcul du filtre passe-bas
   unsigned int next_to_remove4; // pour le calcul du filtre passe-bas
 #elif ALGO_VERSION_PASSE_BAS==2
   int n_passe_bas; // compteur de boucle
   unsigned short somme_passe_bas; // somme pour la moyenne
   unsigned int next_to_remove; // prochain nombre � retirer de la somme
 #elif ALGO_VERSION_PASSE_BAS==1
   int n_passe_bas; // compteur de boucle
   int j; // compteur de boucles
   unsigned short somme_passe_bas; // somme pour la moyenne
 #endif  



  /* calcul de l'histogramme
     ----------------------- */
  #if ALGO_VERSION_HISTO==3
  // v3 : 33166 cycles (130x96) (d�roulement de boucle 4x)
  for (n_histo=TAILLE_HISTOGRAMME-1; n_histo>=0; n_histo--) histogramme[n_histo]=0;
  raster_temp=(unsigned int*)raster;  
  for (n_histo=0; n_histo<nbr_pixels/4; n_histo++) {
    word1 = raster_temp[0];
    raster_temp++;
    i1_1 = (word1     ) & 0xFF;
    i1_2 = (word1 >>8 ) & 0xFF;
    i1_3 = (word1 >>16) & 0xFF;
    i1_4 = (word1 >>24)       ;
    h1 = histogramme[i1_1];
    h2 = histogramme[i1_2];
    h3 = histogramme[i1_3];
    h4 = histogramme[i1_4];
    histogramme[i1_1] = h1+1;
    histogramme[i1_2] = h2+1;
    histogramme[i1_3] = h3+1;
    histogramme[i1_4] = h4+1;
  }
  // finir avec les 0 � 3 derniers pixels de l'image (pas obligatoire puisqu'la pupille n'y est certainement pas)
  /*index_dernier_pixel = (nbr_pixels>>2)<<2;
  for (n_histo=index_dernier_pixel; n_histo<nbr_pixels; n_histo++) {
    histogramme[raster[n_histo]]++;
  } */

  #elif ALGO_VERSION_HISTO==2
  // v2 : 34751 cycles (130x96) (d�roulement de boucle 8x)
  for (n_histo=TAILLE_HISTOGRAMME-1; n_histo>=0; n_histo--) histogramme[n_histo]=0;
  raster_temp = (unsigned int*)raster;
  // ATTENTION : il faudrait prendre en compte tous les pixels (ici les 0..7 derniers pixels ne sont pas pris en compte, suivant le nombre de pixels de l'image)
  for (n_histo = 0; n_histo<nbr_pixels/8; n_histo++) { // calcul de l'histogramme (lecture de 8 pixels � la fois)
    word1 = raster_temp[0]; // lecture des 4 premiers pixels
    word2 = raster_temp[1]; // lecture des 4 seconds pixels (en //)
    raster_temp += 2;
    i1_1 = (word1     ) & 0xFF; // extraction des 8 pixels
    i1_2 = (word1 >>8 ) & 0xFF;
    i1_3 = (word1 >>16) & 0xFF;
    i1_4 = (word1 >>24)       ;
    i2_1 = (word2     ) & 0xFF;
    i2_2 = (word2 >>8 ) & 0xFF;
    i2_3 = (word2 >>16) & 0xFF;
    i2_4 = (word2 >>24)       ;

    h1 = histogramme[i1_1]; // mise � jour de l'histogramme (pas de ++ pour maximiser le //)
    h2 = histogramme[i1_2];
    h3 = histogramme[i1_3];
    h4 = histogramme[i1_4];
    histogramme[i1_1] = h1+1;
    histogramme[i1_2] = h2+1;
    histogramme[i1_3] = h3+1;
    histogramme[i1_4] = h4+1;
    h5 = histogramme[i2_1];
    h6 = histogramme[i2_2];
    h7 = histogramme[i2_3];
    h8 = histogramme[i2_4];
    histogramme[i2_1] = h5+1;
    histogramme[i2_2] = h6+1;
    histogramme[i2_3] = h7+1;
    histogramme[i2_4] = h8+1;     
  }

  
  #elif ALGO_VERSION_HISTO==1
  // v1 : 89304 cycles (130x96)
  for (n_histo=TAILLE_HISTOGRAMME-1; n_histo>=0; n_histo--) histogramme[n_histo]=0;
  for (n_histo = 0; n_histo<nbr_pixels; n_histo++) {
    pixel = raster[n_histo];
    histogramme[pixel]=histogramme[pixel]+1;   //  pas de changement avec a=a+1
  }         
  #endif

  /*
  // Ca fait planter le debugger (crash)
  for (n=TAILLE_HISTOGRAMME*4-1; n>=0; n--) histogramme2_temp[n]=0;
  nbr_pixels_8 = ((nbr_pixels+7)/8)*8; // rendre nbr_pixels multiple de 8
  histogram(raster, nbr_pixels_8, +1, histogramme2, histogramme2_temp);
  */


  /* calcul de l'histogramme cumul�
     ------------------------------ */
  #if ALGO_VERSION_CUMUL_HISTO==4
  // v4 : 569 cycles (d�roulement de boucle+optimisation des sommes)
  somme_cumulee=0;
  for (n_cumul=0; n_cumul<TAILLE_HISTOGRAMME; n_cumul+=4) {
    s0 = histogramme[n_cumul+0];
    s1 = histogramme[n_cumul+1];
    s2 = histogramme[n_cumul+2];
    s3 = histogramme[n_cumul+3];  
    ss0 = somme_cumulee+s0;    
    histogramme_cumule[n_cumul+0]=ss0;
    histogramme_cumule[n_cumul+1]=ss0+s1;
    histogramme_cumule[n_cumul+2]=ss0+s1+s2;
    histogramme_cumule[n_cumul+3]=ss0+s1+s2+s3;
    somme_cumulee = ss0+s1+s2+s3;
  }


  #elif ALGO_VERSION_CUMUL_HISTO==3
  // v3 : 570 cycles (d�roulement de boucle)
  somme_cumulee=0;
  for (n_cumul=0; n_cumul<TAILLE_HISTOGRAMME; n_cumul+=4) {
    s0 = histogramme[n_cumul+0];
    s1 = histogramme[n_cumul+1];
    s2 = histogramme[n_cumul+2];
    s3 = histogramme[n_cumul+3];
    histogramme_cumule[n_cumul+0]=somme_cumulee+s0;
    histogramme_cumule[n_cumul+1]=somme_cumulee+s0+s1;
    histogramme_cumule[n_cumul+2]=somme_cumulee+s0+s1+s2;
    histogramme_cumule[n_cumul+3]=somme_cumulee+s0+s1+s2+s3;
    somme_cumulee += s0+s1+s2+s3;
  }


  #elif ALGO_VERSION_CUMUL_HISTO==2
  // v2 : 572 cycles (somme cumul�e en registre)
  somme_cumulee=0;
  for (n_cumul=0; n_cumul<TAILLE_HISTOGRAMME; n_cumul++) {
    somme_cumulee += histogramme[n_cumul];
    histogramme_cumule[n_cumul]=somme_cumulee;
  }


  #elif ALGO_VERSION_CUMUL_HISTO==1
  // v1 : 1919 cycles
  histogramme_cumule[0]=histogramme[0];
  for (n_cumul=1; n_cumul<TAILLE_HISTOGRAMME; n_cumul++) {
    histogramme_cumule[n_cumul]=histogramme_cumule[n_cumul-1]+histogramme[n_cumul];
  }
  #endif


  /* filtre m�dian */
  #if ORDRE_FILTRE_MEDIAN==5
  // v2 : 21370 cycles (d�roulement de boucle pour n=5)
  for (n_median=0; n_median<ORDRE_FILTRE_MEDIAN_DIV2; n_median++) histogramme_filtre[n_median]=0;
  for (n_median=TAILLE_HISTOGRAMME-ORDRE_FILTRE_MEDIAN_DIV2; n_median<TAILLE_HISTOGRAMME; n_median++) histogramme_filtre[n_median]=0;
  r0=histogramme[0];
  r1=histogramme[1];
  r2=histogramme[2];
  r3=histogramme[3];
  r4=histogramme[4];
  for (n_median=ORDRE_FILTRE_MEDIAN_DIV2; n_median<TAILLE_HISTOGRAMME-ORDRE_FILTRE_MEDIAN_DIV2; n_median++) {
    //copie des valeurs
    r5=histogramme[n_median+4-ORDRE_FILTRE_MEDIAN_DIV2];
    r10=0; // nbr de valeurs tri�es
    
    //tri des valeurs
    // recherche de la 1�re plus petite valeur
    //max=0; // max est la valeur de la 1�re case
    min01234=r0;
    if (r1<=min01234) min01234 = r1;
    if (r2<=min01234) min01234 = r2;
    if (r3<=min01234) min01234 = r3;
    if (r4<=min01234) min01234 = r4;
    if (r0==min01234) r10++;
    if (r1==min01234) r10++;
    if (r2==min01234) r10++;
    if (r3==min01234) r10++;
    if (r4==min01234) r10++;
                                         //min01234 est la plus petite valeur du tableau
                                         // donc elle devrait aller dans la 1�re case
    // recherche de la 2�me plus petite valeur
    if (r10<=2) {
      max=min01234; // max est la valeur de la 1�re case
      min01234=0xFFFF;
      if ((r0>max) && (r0<=min01234)) min01234 = r0;
      if ((r1>max) && (r1<=min01234)) min01234 = r1;
      if ((r2>max) && (r2<=min01234)) min01234 = r2;
      if ((r3>max) && (r3<=min01234)) min01234 = r3;
      if ((r4>max) && (r4<=min01234)) min01234 = r4;
      if (r0==min01234) r10++;
      if (r1==min01234) r10++;
      if (r2==min01234) r10++;
      if (r3==min01234) r10++;
      if (r4==min01234) r10++;
                               // min01234 est maintenant la 2�me plus petite valeur du tableau
                               // donc elle devrait aller dans la 2�me case
      // recherche de la 3�me plus petite valeur du tableau
      if (r10<=2) {
        max=min01234; // max est la valeur de la 2�re case
        min01234=0xFFFF;
        if ((r0>max) && (r0<=min01234)) min01234 = r0;
        if ((r1>max) && (r1<=min01234)) min01234 = r1;
        if ((r2>max) && (r2<=min01234)) min01234 = r2;
        if ((r3>max) && (r3<=min01234)) min01234 = r3;
        if ((r4>max) && (r4<=min01234)) min01234 = r4;
                                // min01234 est maintenant la 3�me plus petite valeur du tableau
                                // donc elle devrait aller dans la troisi��me case
      }//end if
    }//end if        
    
    // d�calage des valeurs pour le prochain bin de l'histogramme
    r0=r1;
    r1=r2;
    r2=r3;
    r3=r4;
    r4=r5;                                                       
    
    histogramme_filtre[n_median]=min01234; // pour un filtre m�dian n=5, la case du milieu est la 3�me CQFD
  }//end for


  #else   // filtre m�dian d'ordre != 5   
  // v1 : 35324 cycles (tri � bulles)
  for (i=0; i<ORDRE_FILTRE_MEDIAN_DIV2; i++) histogramme_filtre[i]=0;
  for (n=TAILLE_HISTOGRAMME-ORDRE_FILTRE_MEDIAN_DIV2; n<TAILLE_HISTOGRAMME; n++) histogramme_filtre[n]=0;
  for (i=ORDRE_FILTRE_MEDIAN_DIV2; i<TAILLE_HISTOGRAMME-ORDRE_FILTRE_MEDIAN_DIV2; i++) {
    //copie des valeurs
    for (j=0; j<ORDRE_FILTRE_MEDIAN; j++) {
      index = i+j-ORDRE_FILTRE_MEDIAN_DIV2;
      temp_median = histogramme[index];
      filtre_median[j]=temp_median;
    }//end for
    //tri des valeurs (juste la moiti�, �a suffit puisqu'on prend l'�l�ment milieu)
    for (k=0; k<ORDRE_FILTRE_MEDIAN_DIV2; k++) {
      for (m=k+1; m<ORDRE_FILTRE_MEDIAN; m++) {
        if (filtre_median[m]<filtre_median[k]) {
          // �change des valeurs
          temp_median = filtre_median[k];
          filtre_median[k] = filtre_median[m];
          filtre_median[m] = temp_median;
        }
      }
    }
    histogramme_filtre[i]=filtre_median[ORDRE_FILTRE_MEDIAN_DIV2]; //filtre m�dian
  }//end for
  #endif

  

  /* filtre passe-bas (moyenne)
     -------------------------- */
  #if ALGO_VERSION_PASSE_BAS==3
  // v3 : 873 cycles (d�roulement de boucle pour n=3)
  #if ORDRE_FILTRE_PASSE_BAS_DIV_2==1
  // pour le filtre d'ordre 3, il n'y a pas besoin de boucle
  histogramme[0]=0;
  histogramme[255]=0;
  #else
  for (n_passe_bas=0; n_passe_bas<ORDRE_FILTRE_PASSE_BAS_DIV2; n_passe_bas++) histogramme[n_passe_bas]=0;
  for (n_passe_bas=(TAILLE_HISTOGRAMME-ORDRE_FILTRE_PASSE_BAS_DIV2); n_passe_bas<TAILLE_HISTOGRAMME; n_passe_bas++) histogramme[n_passe_bas]=0;
  #endif
  next_to_remove1 = histogramme_filtre[0];
  next_to_remove2 = histogramme_filtre[1];
  next_to_remove3 = histogramme_filtre[2];
  somme_passe_bas = next_to_remove1+next_to_remove1+next_to_remove2;
  for (n_passe_bas=ORDRE_FILTRE_PASSE_BAS_DIV2; n_passe_bas<(TAILLE_HISTOGRAMME-ORDRE_FILTRE_PASSE_BAS_DIV2); n_passe_bas++) {
    next_to_remove4 = histogramme_filtre[n_passe_bas+ORDRE_FILTRE_PASSE_BAS_DIV2];
    somme_passe_bas += next_to_remove3;
    somme_passe_bas -= next_to_remove1;
    next_to_remove1 = next_to_remove2;
    next_to_remove2 = next_to_remove3;
    next_to_remove3 = next_to_remove4;
    histogramme[n_passe_bas]=somme_passe_bas;
  }//end for
  
  
  #elif ALGO_VERSION_PASSE_BAS==2
  // v2 : 890 cycles
  somme_passe_bas = histogramme_filtre[0];
  next_to_remove = histogramme_filtre[0];
  for (n_passe_bas=0; n_passe_bas<ORDRE_FILTRE_PASSE_BAS-1; n_passe_bas++) { // initialiser le filtre (somme des n premiers �l�ments)
      somme_passe_bas += histogramme_filtre[n_passe_bas];
  }//end for                          
  // maintenant : somme_passe_bas = 2*h[0]+h[1..n-2]
  #if ORDRE_FILTRE_PASSE_BAS_DIV_2==1
  // pour le filtre d'ordre 3, il n'y a pas besoin de boucle
  histogramme[0]=0;
  histogramme[255]=0;
  #else
  for (n_passe_bas=0; n_passe_bas<ORDRE_FILTRE_PASSE_BAS_DIV2; n_passe_bas++) histogramme[n_passe_bas]=0;
  for (n_passe_bas=(TAILLE_HISTOGRAMME-ORDRE_FILTRE_PASSE_BAS_DIV2); n_passe_bas<TAILLE_HISTOGRAMME; n_passe_bas++) histogramme[n_passe_bas]=0;
  #endif
  for (n_passe_bas=ORDRE_FILTRE_PASSE_BAS_DIV2; n_passe_bas<(TAILLE_HISTOGRAMME-ORDRE_FILTRE_PASSE_BAS_DIV2); n_passe_bas++) {
    // somme des �chantillons
    somme_passe_bas+=histogramme_filtre[n_passe_bas+ORDRE_FILTRE_PASSE_BAS_DIV2-1]; // on ajoute le nouvel �l�ment de la moyenne
    somme_passe_bas-=next_to_remove; // on enl�ve le plus ancien �l�ment de la moyenne
    next_to_remove = histogramme_filtre[n_passe_bas-ORDRE_FILTRE_PASSE_BAS_DIV2];
    histogramme[n_passe_bas]=(somme_passe_bas+ORDRE_FILTRE_PASSE_BAS-1)/ORDRE_FILTRE_PASSE_BAS_UPPER_POW2;  //filtre passe bas (+ordre_filtre_passe_bas-1 pour arrondir � 1)
      // on divise par une puissance de 2 parce que du point de vue de l'histogramme, �a ne change rien (histogramme plus tass�)
      // par contre, la division est remplac�e par un shift => plus rapide
  }//end for

    
  #elif ALGO_VERSION_PASSE_BAS==1
  // v1 : 9552 cycles
  for (n_passe_bas=ORDRE_FILTRE_PASSE_BAS_DIV2; n_passe_bas<(TAILLE_HISTOGRAMME-ORDRE_FILTRE_PASSE_BAS_DIV2); n_passe_bas++) {
    // somme des �chantillons
    somme_passe_bas=0;
    for (j=-ORDRE_FILTRE_PASSE_BAS_DIV2; j<=ORDRE_FILTRE_PASSE_BAS_DIV2; j++) {
      somme_passe_bas += histogramme_filtre[n_passe_bas+j];
    }//end for
    histogramme[n_passe_bas]=(somme_passe_bas+ORDRE_FILTRE_PASSE_BAS-1)/ORDRE_FILTRE_PASSE_BAS;  //filtre passe bas (+ordre_filtre_passe_bas-1 pour arrondir � 1)
  }//end for
  #endif
}/* -----------------------------------------------------
end init_recherche_seuil_optimal */




           
           
           
im_hist_get_lobe(unsigned short* histogramme,
                 unsigned char num_lobe,
                 unsigned char sens,
                 unsigned char* b_inf,
                 unsigned char* b_sup) {
/* ---------------------------------------------------------------------
 * retourne les bornes du lobe sp�cifi�
 * init_recherche_seuil_optimal() doit avoir ete appel� avant
 *
 * Input : 
 *   param�tres de la fonction
 * Output :
 *   histogramme (modifi�)
 *   b_sup         
 *   b_inf
  */
  unsigned short i;
  unsigned char num_lobe_int; // compteur de lobe

  if (sens==0) {
    // recherche du lobe depuis la couleur 0 de l'histogramme

    // filtrage des couleurs non significatives (nbr de pixels trop faibles)
    for (i=0; i<TAILLE_HISTOGRAMME; i++) {  // 279 cycles
      if (histogramme[i]<=1) histogramme[i] = 0;
    }//end for
  
    //recherche du d�but du premier lobe (gradient!=0)
    i=0;
    while (histogramme[i]==0) {
      i++;
    }//end while
  
    // recherche du n-�me lobe
    for (num_lobe_int=num_lobe; num_lobe_int>0; num_lobe_int--) {
      *b_inf = i-1; // d�but du lobe
   
      // recherche du sommet du lobe (mont�e)
      while (i<256 && histogramme[i]>=histogramme[i-1]) {
        i++;
      }//end while
      // recherche de la fin du lobe (descente)
      while (i<256 && histogramme[i]<histogramme[i-1]) { 
        i++;
      }//end while
      *b_sup = i-1; // fin du lobe
    }//end while

   
  } else {
    // recherche du lobe � partir de la couleur 255->0
    ////////////////////////////////////////////////////////////////////////
 
    // correction d'histogramme n�cessaire car les lampes sont souvent petites (i.e. peu de pixels)
    for (i=0; i<TAILLE_HISTOGRAMME; i++) {
      if (histogramme[i]!=0) {
        if (histogramme[i]<=6) {
          histogramme[i]=6;
        }//end if
      }//end if
    }//end for

    // recherche du d�but du premier lobe
    i=254;  // fin de l'histogramme 
    while (histogramme[i]==0) {
      i--;
    }//end while

    // recherche du n-�me lobe
    num_lobe_int = 1;
    while (num_lobe_int<=num_lobe) {
      *b_sup = i+1; // d�but du lobe
        // recherche du sommet du lobe (mont�e)
      while (i>0 && histogramme[i]>=histogramme[i+1]) {
        i--;
      }//end while
      // recherche de la fin du lobe (descente)
      while (i>0 && histogramme[i]<histogramme[i+1]) {
        i--;
      }//end while
      *b_inf = i+1;
  
      // passer au lobe suivant
      num_lobe_int++;
    }//end while

  }//end if
    
}/* ---------------------------------------------------------------------------
end im_hist_get_lobe */









calculer_seuil_optimal(unsigned char* b_inf,
                       unsigned char* b_sup) {
/* -----------------------------------------------------------------------------
 * Calcul du seuil optimal pour la segmentation de l'image � partir
 * de l'histogramme cumul� et des bornes du premier lobe.
 *
 * Proc�de de mani�re dichotomique jusqu'� optiner une image seuill�e contenant
 * moins de 10% des pixels � '1'.
 *
 * Input : 
 *   param�tres : b_inf, b_sup
 *   var. globales : nbr_lignes, nbr_colonnes, histogramme_cumule
 * Output :
 *   b_inf_out, b_sup_out
 */
  unsigned int limite = nbr_lignes*nbr_colonnes/10; // limite = 10 % des pixels
  short m, m2; // index de couleur du point milieu (pour la dichotomie)
  unsigned char b_inf2, b_sup2; // index de couleur pour la dichotomie
  char i; // compteur de boucle
  
  // correction du seuil (pour les images mal illumin�es)
  //image mal illumin�e : plus de x pixels<seuil (x est d�fini en % de la taille de l'image)
  b_inf2 = *b_inf;
  b_sup2 = *b_sup;
  
  m = -1;
  for (i=7; i>0; i--) {  // max 7 dichotomies car couleur_max=env 128 et 2^7=128
    m2 = m;
    m=(b_sup2+b_inf2+1)/2; // +1 pour l'arrondi
    if (m==m2) break; // le seuil n'a pas chang� => on le garde
    if ((histogramme_cumule[m]-histogramme_cumule[*b_inf])>limite) {
      // on est en dessus de la limite
      b_sup2 = m;
      //System.out.println((histogramme_cumule[m]-histogramme_cumule[*b_inf])+" pixels sous le seuil indiqu�");
    } else {
      if ((histogramme_cumule[m]-histogramme_cumule[*b_inf])<limite) {
        // on est en dessous de la limite
        b_inf2 = m;
        //System.out.println((h_cumul[m]-h_cumul[*b_inf])+" pixels sous le seuil indiqu�");
      } else {
        // on est au bon compte => OK
        break;
      }//end if
    }//end if
  }//end for
  *b_sup = m;
  //System.out.println(histogramme_cumule[*b_sup]+" pixels sous le seuil indiqu�");
}/* ---------------------------------------------------------------------
end calculer_seuil_optimal */


                                                     
                                                     
                                                     


seuillage(unsigned char* image,
          unsigned char* image_seuillee,
          unsigned int   n,
          unsigned char  seuil) {
/* ------------------------------------------------------------------------
 * Effectue le seuillage de l'image. ('image seuillee' a les pixels � '1'
 * pour toutes les couleurs < 'seuil').
 * 'n' est le nombre de pixels de l'image         
 */
 unsigned int i;

 for (i=0; i<n; i++) {
   image_seuillee[i]=image[i]<seuil; // 1 si image[0]<seuil, 0 sinon
 }
}/* ----------------------------------------------------------------------
end seuillage */


                                                   
                                                   
                                                   



get_barycentre(unsigned char* image_seuil,      
               short nbr_colonnes,
               short nbr_lignes,
               short x_inf,
               short x_sup,
               short y_inf,
               short y_sup,
               float* centre_x,
               float* centre_y) {
/* ----------------------------------------------------------------------------
 * calcule le barycentre entre les pixels (x_inf, y_inf) et (x_sup, y_sup)
 * Le centre calcul� est donn� dans (centre_x, centre_y)
 */

  // variables
  int somme_x = 0; // somme des coordonn�es X des pixels � '1'
  int somme_y = 0; // somme des coordonn�es Y des pixels � '1'
  int nbr_pixels = 0; // nombre de pixels � '1' sur l'image
  int num_colonne, num_ligne; // compteurs de boucles
  int i; // index du pixel                  
  
  i = y_inf * nbr_colonnes;
  for (num_ligne=y_inf; num_ligne<y_sup; num_ligne++) {
    for (num_colonne=x_inf; num_colonne<x_sup; num_colonne++) {
      if (image_seuil[i+num_colonne]==1) {
        somme_x += num_colonne;
        somme_y += num_ligne;
        nbr_pixels++;
      }//end if
    }//end for (num_colonne)
    i += nbr_colonnes; // passer � la ligne suivante
  }//end for (num_lignes)
                          
  if (nbr_pixels == 0) {
    // aucun pixel dans l'image => centre = [-1 -1]  (sinon il y a une division par 0)
    *centre_x = -1;
    *centre_y = -1;
  } else {
    // calcul du centre
    *centre_x = (float)somme_x / (float)nbr_pixels;
    *centre_y = (float)somme_y / (float)nbr_pixels;
  }//end if

}/* -------------------------------------------------------------------------
end get_barycentre */








get_barycentre_seuil(unsigned char* image,      
               short nbr_colonnes,
               short nbr_lignes,
               short x_inf,
               short x_sup,
               short y_inf,
               short y_sup,
               float* centre_x,
               float* centre_y,
               unsigned char seuil) {
/* ----------------------------------------------------------------------------
 * calcule le barycentre entre les pixels (x_inf, y_inf) et (x_sup, y_sup)
 * Le centre calcul� est donn� dans (centre_x, centre_y)
 *
 * Le calcul ne se fait pas sur l'image seuill�e mais sur l'image originale 
 * avec indication du seuil
 */

  // variables
  int somme_x = 0; // somme des coordonn�es X des pixels � '1'
  int somme_y = 0; // somme des coordonn�es Y des pixels � '1'
  int nbr_pixels = 0; // nombre de pixels � '1' sur l'image
  int num_colonne, num_ligne; // compteurs de boucles
  int i; // index du pixel

 
/*  
  unsigned int* raster_temp;                  
  unsigned int word1;       
  unsigned char i1_1, i1_2, i1_3, i1_4;
  unsigned int inc_x;
  unsigned int cmp1, cmp2, cmp3, cmp4;

  i = y_inf * nbr_colonnes;
  // x_inf/sup doivent �tre multiples de 8
  x_inf = x_inf/4;
  x_sup = x_sup/4;
  for (num_ligne=y_inf; num_ligne<y_sup; num_ligne++) {
    raster_temp = (unsigned int*)raster + i;
    for (num_colonne=x_inf; num_colonne<x_sup; num_colonne++) {
      word1 = raster_temp[0];
      raster_temp++;             
      inc_x = num_colonne*4;
      i1_1 = (word1     ) & 0xFF; // extraction des 8 pixels
      i1_2 = (word1 >>8 ) & 0xFF;
      i1_3 = (word1 >>16) & 0xFF;
      i1_4 = (word1 >>24)       ;
      cmp1 = i1_1 < seuil;
      cmp2 = i1_2 < seuil;
      cmp3 = i1_3 < seuil;
      cmp4 = i1_4 < seuil;
      if (cmp1) {
        somme_x += inc_x+0;
        somme_y += num_ligne;
        nbr_pixels++;
      }//end if
      if (cmp2) {
        somme_x += inc_x+1;
        somme_y += num_ligne;
        nbr_pixels++;
      }//end if
      if (cmp3) {
        somme_x += inc_x+2;
        somme_y += num_ligne;
        nbr_pixels++;
      }//end if
      if (cmp4) {
        somme_x += inc_x+3;
        somme_y += num_ligne;
        nbr_pixels++;
      }//end if
    }//end for (num_colonne)
    i += nbr_colonnes; // passer � la ligne suivante
  }//end for (num_lignes)
*/

  i = y_inf * nbr_colonnes;
  for (num_ligne=y_inf; num_ligne<y_sup; num_ligne++) {
    for (num_colonne=x_inf; num_colonne<x_sup; num_colonne++) {
      if (image[i+num_colonne]<seuil) {
        somme_x += num_colonne;
        somme_y += num_ligne;
        nbr_pixels++;
      }//end if
    }//end for (num_colonne)
    i += nbr_colonnes; // passer � la ligne suivante
  }//end for (num_lignes)

  if (nbr_pixels == 0) {
    // aucun pixel dans l'image => centre = [-1 -1]  (sinon il y a une division par 0)
    *centre_x = -1;
    *centre_y = -1;
  } else {
    // calcul du centre
    *centre_x = (float)somme_x / (float)nbr_pixels;
    *centre_y = (float)somme_y / (float)nbr_pixels;
  }//end if

}/* -------------------------------------------------------------------------
end get_barycentre_seuil */













add_ligne(unsigned char* image_sous_ech, unsigned char* image_sous_ech_marque,
          unsigned int Xm, unsigned int Ym,
          int x, int y,
          unsigned int  x_inf,  unsigned int  x_sup,  unsigned int  y_inf,  unsigned int  y_sup,
          unsigned int* x_inf2, unsigned int* x_sup2, unsigned int* y_inf2, unsigned int* y_sup2,
          int* surface_zone) {
/* -----------------------------------------------------------------------------
*/
    // retourne [x_inf x_sup y_inf y_sup surface_zone] 
    //System.out.println("  add_ligne("+x+" "+y+")");
    int xmin=x;
    int xmax=x;
    int xx; // index x du pixel voisin d'une nouvelle ligne (plus haute ou plus basse)
    int surface_haut, surface_bas; // surface en haut/en bas de la la ligne courante

    
    //recherche de la borne gauche de la ligne
    while (image_sous_ech[xmin+Xm*y]==1 && image_sous_ech_marque[xmin+Xm*y]==0) {
      image_sous_ech_marque[xmin+Xm*y]=1;
      xmin--;
      if (xmin<0) break;
    }//end while
    xmin++;

    //recherche de la borne droite de la ligne
    image_sous_ech_marque[xmax+Xm*y]=0;
    while (image_sous_ech[xmax+Xm*y]==1 && image_sous_ech_marque[xmax+Xm*y]==0) {
      image_sous_ech_marque[xmax+Xm*y]=1;
      xmax++;
      if (xmax>=Xm) break;
    }//end while
    xmax--;


    *surface_zone = xmax-xmin+1;
    //System.out.println("  xmin="+xmin+" xmax="+xmax+" longueur_ligne="+surface_zone);

    //System.out.println(x_inf+" "+x_sup+" "+y_inf+" "+y_sup);
    //System.out.println(xmin+" "+xmax+" "+y+" "+y);

    //calcul des bornes de la zone
    if (xmax>x_sup) x_sup=xmax;
    if (xmin<x_inf) x_inf=xmin;
    if (y>y_sup) y_sup=y;
    if (y<y_inf) y_inf=y;

    //System.out.println(x_inf+" "+x_sup+" "+y_inf+" "+y_sup);
    
    // toute la ligne est maintenant marqu�e. On va la parcourir pour essayer
    // de trouver un pixel au dessus ou au dessous qui ne soit pas marqu�, ce
    // qui permettra de trouver une autre ligne
    xx = xmin;
    while (xx<=xmax) {
      if (y-1>=0) {
        if (image_sous_ech_marque[xx+Xm*(y-1)]==0 && image_sous_ech[xx+Xm*(y-1)]==1) {
          add_ligne(image_sous_ech, image_sous_ech_marque, 
                    Xm, Ym,
                    xx,y-1,
                    x_inf,   x_sup,  y_inf,  y_sup,
                    &x_inf, &x_sup, &y_inf, &y_sup,
                    &surface_haut);
          *surface_zone += surface_haut;
        }//end if
      }//end if
      if (y+1<Ym) {
        if (image_sous_ech_marque[xx+Xm*(y+1)]==0 && image_sous_ech[xx+Xm*(y+1)]==1) {
          add_ligne(image_sous_ech, image_sous_ech_marque, 
                    Xm, Ym,
                    xx,y+1,
                    x_inf,   x_sup,  y_inf,  y_sup,
                    &x_inf, &x_sup, &y_inf, &y_sup,
                    &surface_bas);
          *surface_zone += surface_bas;
        }//end if
      }//end if
      xx += 1;
    }//end while

  *x_inf2 = x_inf;
  *x_sup2 = x_sup;
  *y_inf2 = y_inf;
  *y_sup2 = y_sup;


    //System.out.println("result="+x_inf+" "+x_sup+" "+y_inf+" "+y_sup);
}/* ---------------------------------------------------------------------------
end add_ligne */





            
            
get_barycentre_id_zones(unsigned char* A,
                        unsigned char seuil,
                        float* centre_x,
                        float* centre_y) {
/* -----------------------------------------------------------------------------
 * Recherche du barycentre mais avec une identification de zones pr�alable
 *
 * Le nombre de zones maximal est sp�cifi� par NBR_ZONES_MAX. Si l'image
 * sous-�chantillonn�e contient davantage de zones, les moins bonnes zones
 * sont remplac�es (l'algorithme est dans ce cas plus lent puisqu'il faut 
 * rechercher les zones � remplacer). Par cons�quent, si il y a suffisamment
 * de m�moire, il y a int�r�t � avoir un nombre de zones potentielles le plus
 * grand possible.
 */                     
  // taille_carre = taille du carr� de sous-�chantillonnage
  //                (puissance de 2 pour une vitesse optimale)
  //                8 est un bon compromis entre le temps de calcul et la
  //                capacit� de d�tecter des zones
  #define taille_carre 8
  // pourcentage = pourcentage de remplissage du bloc pour que le pixel sous-ech
  //                 0='1' si   0% de pixels
  //               128='1' si 100% de pixels
  //                (puissance de 2 pour une vitesse optimale)
  //               128 donne de tr�s bon r�sultats sur l'erreur de type 1
  #define pourcentage 20
  unsigned short XX2 = nbr_colonnes/taille_carre;
  unsigned short YY2 = nbr_lignes  /taille_carre;
  
  // variables pour le sous-�chantillonnage
  unsigned short x,y,xx,yy,x2,y2; // compteur de boucles
  unsigned char etat_element; // �tat du pixel sous-�ch. ('0' ou '1')
  unsigned short nbr_elements_1; // nombre d'�l�ments � '1' dans le bloc

  // variables pour la recherche de zones
  // x, y, taille_zone, surface_zone, bornes (stock�es pour le calcul du barycentre)
  //unsigned int i; // compteur de boucles
  unsigned short nbr_zones = 0; //nombre de zones d�couvertes sur l'image sous-�chantillonn�e  
  //float taille_zone; // taille de la diagonale de la zone en pixels
  //int tmp_zone; // num�ro de zone temporaire pour l'�change (tri)
  unsigned int x_inf, x_sup, y_inf, y_sup; // bornes de la zone
  int x_inf2, x_sup2, y_inf2, y_sup2; // bornes de clipping 
  int surface_zone; // surface de la zone en pixels
  unsigned int i; // compteur de boucle
  unsigned int num_zone; // num�ro de la zone courante

  // variables pour la recherche de la meilleure zone
  float meilleur_rapport;
  unsigned int meilleure_zone;                       
  

  // Sous �chantillonnage    
  // 24246 cycles (130x96)
  for (yy=0; yy<YY2; yy++) {
    for (xx=0; xx<XX2; xx++) {
      etat_element=1; // �tat de l'�l�ment (par d�faut, il est allum�)
      nbr_elements_1 = 0;
      //adr_y = 0;
      for (y2=0; y2<taille_carre; y2++) {
        for (x2=0; x2<taille_carre; x2++) {
          if (A[xx*taille_carre + x2+nbr_colonnes*(yy*taille_carre + y2)]<seuil) {
            nbr_elements_1++;
          }
        }//end for x2
      }//end for y2
      // si x % des pixels du bloc sont � '1' alors le pixel
      // sous-�chantillonn� est mis � '1'
      if (nbr_elements_1<pourcentage*taille_carre*taille_carre/128) etat_element=0; 
      image_sous_ech[xx+XX2*yy]=etat_element;
      image_sous_ech_marque[xx+XX2*yy]=0; // initialiser le masque
    }//end for y 
  }//end for x
 
  // Apr�s sous �chantillonnage, il ne reste que quelques zones (normalement
  // moins que dans l'image seuill�e initiale, id�alement une zone unique
  // correspondant � la pupille). Le nombre de pixels �tant relativement
  // r�duit, il est possible de caract�riser les zones en dimension :
  // plus la forme de la zone s'�loigne d'un disque, plus le rapport taille/surface
  // est petit. Si l'on classe les zones en fonction de leur rapport taille/surface,
  // on peut d�terminer la pupille en choississant la zone ayant le rapport le plus
  // grand (en r�alit�, le rapport le plus grand serait un carr�). La taille est
  // d�finie par SQRT((xmax-xmin)^2+(ymax-ymin)^2).
  //
  // Pour identifier une zone, on proc�de de la mani�re suivante :
  //  1) cr�ation d'une table des pixels d�j� examin�s (=marqu�).
  //  2) parcours de l'image sous �chantillonn�e jusqu'� un pixel � '1' non marqu�
  //  3) parcours de la zone (connexit� 4) en marquant les pixels � '1', de mani�re r�cursive
  //     (Le nombre de r�cursions est au maximum de m, m �tant la taille de l'image sous-�ch)
  //
  // Ensuite, on connait le pixel de d�part de la zone dans les coordonn�e de
  // l'image sous-�chantillonn�e et il ne reste plus qu'� identifier la zone 
  // correspondante dans l'image seuill�e. Le barycentre n'est alors calcul� que
  // sur une seule zone.
  //
  // La complexit� de cet algorithme est de l'ordre d'un parcours d'image pour
  // le sous-�chantillonnage, d'un parcours de l'image sous-�chantillonn�e pour
  // la classification des zones et d'un parcours de zone pour la pr�paration 
  // au barycentre. Au total : O(M+m+M/10) avec M la surface de l'image, m la
  // surface �chantillonn�e et M/10 la zone identifi�e comme la pupille (rappel:
  // la pupille fait environ 10% de la surface de l'image). Cela �quivaut �
  // environ 1.2*M op�rations.
  //
  // Au total, on est bien loin de la complexit� de l'�rosion qui est de O(M*N)
  // (env. 3600*M op�rations) : environ 3000 fois moins de calcul.
  //
  x=0;
  y=0;
  
  while (1==1) {
    if (image_sous_ech[x+XX2*y]==1 && image_sous_ech_marque[x+XX2*y]==0) {
      // d�but d'une zone => on traite la zone
        
      // la zone est parcourue en connectivit� 4 :
      //
      //         3
      //       2 x 4
      //         1 
      //System.out.println("nouvelle zone : ");
      add_ligne(image_sous_ech, image_sous_ech_marque, XX2,YY2, x,y,XX2, 0, YY2, 0, &x_inf, &x_sup, &y_inf, &y_sup, &surface_zone);
        
      //taille_zone=sqrtf((float)((x_sup-x_inf+1)*(x_sup-x_inf+1))+(float)((y_sup-y_inf+1)*(y_sup-y_inf+1))); //taille de la diagonale de la zone en pixels

      // sauvegarde des donn�es de la zone
      num_zone = nbr_zones;
      if (nbr_zones>=NBR_ZONES_MAX) {
        // erreur : trop de zones => on remplace la moins bonne zone (rapport le plus �lev�)
        meilleur_rapport = zones_rapport[0];  
        meilleure_zone = 0;
        for (i=1; i<nbr_zones; i++) {// recherche de la moins bonne zone
          if (zones_rapport[i]>meilleur_rapport) {
            meilleure_zone = i;
            meilleur_rapport=zones_rapport[i];
          }//end if
        }//end for i
        
        num_zone = meilleure_zone;
      } else {
        // il reste de la place pour les zones => on la place dans l'ordre
        nbr_zones++;
      }//end if (trop de zones)
      
      //zones_x[num_zone]=x;
      //zones_y[num_zone]=y;
      //zones_taille_zone[num_zone]=taille_zone;
      //zones_surface_zone[num_zone]=surface_zone;
      zones_x_inf[num_zone]=x_inf;
      zones_x_sup[num_zone]=x_sup;
      zones_y_inf[num_zone]=y_inf;
      zones_y_sup[num_zone]=y_sup;
      //zones_rapport[num_zone]=taille_zone/surface_zone;
      zones_rapport[num_zone]=fabsf(surface_zone/(float)((x_sup-x_inf+1)*(y_sup-y_inf+1))-3.1415/(float)4);
      // rapport id�al = Pi/4 pour une ellipse

    } else {
      // pas une nouvelle zone => on continue
      image_sous_ech_marque[x+XX2*y]=1; // on marque la case
      x++; // on passe � la case suivante
      if (x>=XX2) {
        // on passe � la ligne suivante
        x=0;
        y++;
      }//end if
    }//end if
    if (y>=YY2)  break; //toute l'image a �t� trait�e
  }//end while

 
  // tri des zones par rapport croissant (tri � bulles)
  /*
  // tri pas n�cessaire : on utilise seulement la 1�re zone
  for (i=0; i<nbr_zones; i++) zones[i]=i; // initialisation des num�ros de zones
  for (x=0; x<nbr_zones-1; x++) {
    for (y=x+1; y<nbr_zones; y++) {
      if (zones_rapport[zones[y]]<zones_rapport[zones[x]]) { // && zones_surface_zone[zones[x]]<X2*Y2/10
        // �change des valeurs
        tmp_zone = zones[x];
        zones[x]=zones[y];
        zones[y]=tmp_zone;
      }//end if
    }//end for y
  }//end for x   
  meilleure_zone = zones[0]; // la meilleure zone est le premi�re tri�e
  */

  meilleur_rapport = zones_rapport[0];  
  meilleure_zone = 0;
  for (x=1; x<nbr_zones; x++) {
    //if (zones_rapport[x]>meilleur_rapport) {
    if (zones_rapport[x]<meilleur_rapport) {
      meilleure_zone = x;
      meilleur_rapport=zones_rapport[x];
    }//end if
  }//end for x
  
  // affichage des zones
  /*for (x=0; x<nbr_zones; x++) {
    printf("%i %f f i %i %i %i %i\n",x,
                      zones_rapport[x],
                      //zones_taille_zone[x],
                      //zones_surface_zone[x],
                      zones_x_inf[x],
                      zones_x_sup[x],
                      zones_y_inf[x],
                      zones_y_sup[x]);
  }//end for i
   */ 
  //calcul des bornes de la zone de l'image initiale pour le barycentre (+/-1 pour agrandir la zone de recherche)
  // Note : normalement, il faudrait prendre un pixel de la zone sur l'image
  //        seuill�e et faire une recherche de r�gion, ce qui d�termine la bonne
  //        zone. Ici, on fait l'approximation d'une zone carr�e, ce qui ne devrait
  //        pas etre trop faux, compte tenu de la forme de la pupille.
  if (nbr_zones==0) {
    puts("WARNING : Aucune zone trouvee (=> reduire 'taille_carre' ou\n"
         "          diminuer le taux de remplissage du bloc).\n"
         "          Le barycentre a ete calcule sur toute l'image");

    // calcul du barycentre (sur toute l'image puisqu'on a pas de zones)
    get_barycentre_seuil(A, nbr_colonnes, nbr_lignes, 0, nbr_colonnes,0, nbr_lignes, centre_x, centre_y, seuil);
  } else {
    x_inf2 = (zones_x_inf[meilleure_zone]-2)*taille_carre; // +/-2 pour �tre s�r d'avoir la pupille incluse
    x_sup2 = (zones_x_sup[meilleure_zone]+2)*taille_carre;
    y_inf2 = (zones_y_inf[meilleure_zone]-2)*taille_carre;
    y_sup2 = (zones_y_sup[meilleure_zone]+2)*taille_carre;
    if (x_inf2<0)  x_inf2=0;
    if (y_inf2<0)  y_inf2=0;
    if (x_sup2>=nbr_colonnes) x_sup2=nbr_colonnes-1;
    if (y_sup2>=nbr_lignes) y_sup2=nbr_lignes-1;

    // calcul du barycentre (uniquement sur la zone sp�cifi�e)
    //get_barycentre_seuil(A, nbr_colonnes, nbr_lignes, y_inf2, y_sup2,x_inf2, x_sup2, centre_x, centre_y, seuil);
    get_barycentre_seuil(A, nbr_colonnes, nbr_lignes, x_inf2, x_sup2, y_inf2, y_sup2, centre_x, centre_y, seuil);
  }//end if
    
}/* ----------------------------------------------------------------------------
end get_barycentre_id_zones */








terminer() {   /* routine factice pour permettre d'ex�cuter le code jusqu'� la fin par GEL_Go(terminer); */
  static volatile int fake;
  fake++; 
  
}


                                          
main() {     
  FILE* f;  
  time_t temps_initial;
  time_t temps_final;      
  float temps;
 /* #define LED1_on           0x0E000000
  #define LED2_on           0x0D000000
  #define LED3_on           0x0B000000
  #define LEDs_off          0x07000000
  volatile int* LEDs = (int*)0x90080000;
*LEDs = LEDs_off;*/  
  
  
  
  nbr_pixels = nbr_colonnes*nbr_lignes;
  printf("taille = %d x %d\n", nbr_colonnes, nbr_lignes);

  // calcul du seuil optimal pour la segmentation d'image
  time(&temps_initial);             

#define NBR_BOUCLES 100000
  // 5000 boucles en externe, 50000 en interne
  temp=0;
  for (temp=0; temp<NBR_BOUCLES; temp++) {
    init_recherche_seuil_optimal();
    im_hist_get_lobe(histogramme, 1, 0, &b_inf, &b_sup);
    calculer_seuil_optimal(&b_inf, &b_sup);

    // seuillage de l'image                          
    #ifdef ID_ZONES
      get_barycentre_id_zones(raster, b_sup+1, &centre_x, &centre_y);
    #else
      #ifdef CALCULER_IMAGE_SEUILLEE
        seuillage(raster, image_seuil, nbr_pixels, b_sup+1);
        get_barycentre(image_seuil, nbr_colonnes, nbr_lignes, 0, nbr_colonnes,0, nbr_lignes, &centre_x, &centre_y);
      #else
        get_barycentre_seuil(raster, nbr_colonnes, nbr_lignes, 0, nbr_colonnes,0, nbr_lignes, &centre_x, &centre_y, b_sup+1);
      #endif
    #endif

  }//end for

  time(&temps_final);              
  temps = (temps_final-temps_initial)*1000/(float)NBR_BOUCLES;
  printf("duree = %f [ms]\n", temps);

  printf("centre = (%f, %f)\n", centre_x, centre_y);
  
  // ecriture du r�sultat dans un fichier
  f = fopen("c:\\apps\\ti\\myprojects\\eye_tracker\\timing_tms2.txt","a");   // for append
  fprintf(f, "%i\t%i\t%i\t%f\t%f\t%f\t%i\t%i\t%i\t%i\n",num_image, nbr_colonnes, nbr_lignes, temps, centre_x, centre_y, centre_x_db, centre_y_db, diametre_x, diametre_y);
  fflush(f);
  fclose(f);
   
  terminer();
}/* end main */
