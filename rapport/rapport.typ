#import "@preview/cetz:0.2.2"

#set text(size: 1.2em)
#set block(spacing: 1.5em)

// Aligne les images et figure au centre
#set figure(numbering: none)
#show figure: align.with(center)
#show image: align.with(center)

// Titre en gras
#show heading: text.with(size: 1.3em, weight: "bold")
#show heading: it => {
  it.body
  v(0.7em)
}

// Header & Footer
#set page(
  margin: (top: 45pt, bottom: 32pt),

  header: context {
    if counter(page).get().first() > 1 [
      #set text(8pt)
      #smallcaps[Robin Forestier - Benoît Delay]
      #h(1fr) _Lab05 - Traitement des données_
      
    ]
  }, 

  footer: context {
    if counter(page).get().first() > 1 [
      #set align(right)
      #set text(8pt)
      #counter(page).display(
        "1/1",
        both: true,
      )
    ]
  }
)


// Titre du rapport
#align(center + horizon, text(2em)[Laboratoire n° 5 : Traitement des données \ 
						Robin Forestier - Benoît Delay])
#v(1.5em)

#pagebreak()

= Introduction

L’objectif de ce laboratoire est de développer un système qui, utilisant la plateforme DE1-SoC, est
capable de capter et de traiter des données audio et vidéo afin d’en extraire des informations utiles.

Nous examinerons et analyserons également les différents temps d’exécutions des tâches afin d’étudier et d’optimiser les performances temps réel et l’implémentation de celles-ci.

= Description du système

Le système est composé de deux tâches principales : une tâche audio et une tâche vidéo.

La tâche audio à pour but de récupérer un flux audio via un micro et de le copier sur la sortie audio (casque) de la carte. Elle est composée de trois sous tâche.

- Aquisition du flux audio
- Traitement du flux audio (FFT)
- Affichage du temps d'exécution et de la fréquence dominante de la FFT

La tâche vidéo à pour but de récupérer un flux vidéo d'un fichier et de l'afficher à l'écran via la sortie VGA. Elle est aussi est composée de deux sous-tâches.

- Conversion en niveaux de gris
- Convolution (detection de contour)

== Entrées / Sorties

Pour la partie audio, le micro doit être branché sur l'entrée audio de la carte (ROSE). La sortie audio est disponible sur la prise jack de la carte (VERT).

Pour activer ou désactier une tâche, il suffit d'actionner un switch sur la DE-1.

#table(
  columns: 5,
  inset: 10pt,
  align: center + horizon,
  table.header(
    [], [*Convolution*], [*Greyscale*], [*Video task*], [*Audio task*]
  ),
  [*Switch*],
  [9],
  [8],
  [1],
  [0]
)

Et le bouton KEY0 permet d'arreter proprement le programme.

= Caractérisation des tâches

// Veuillez documenter vos mesures (méthode, nombre de mesures, niveau de confiance, etc. . .) et l’approche utilisée afin de déterminer le pire temps d exécution.

Premièrement, nous avons mesuré le temps d'exécution de chaque tache séparément. Cela nous permet d'avoir une idée du temps d'exécution de chaque tâche.

Pour cela nous avons utilisé les timer Xenomai, avec la fonction `rt_timer_read()`. Nous avons effectué `~` 50 mesures pour chaque tâche, et avons calculé la moyenne, la variance et l'écart type.

= Tâche audio

== Mesures

= Tâche vidéo

La tâche vidéo à pour but de récupérer un flux vidéo d'un fichier et de l'afficher à l'écran via la sortie VGA.

Elle est aussi est composée de deux sous-tâches : la conversion en niveaux de gris et la convolution (detection de contour). 

== Mesures

Voici les temps d'exécution de chaque tâche. La mesure à été effectuée sur un système non chargé et seul la tâche vidéo et ioctl était active.

=== Vidéo

```
-----------------------summary1.c -----------------------------
  Total of 44 values 
    Minimum  = 17.475480 (position = 4) 
    Maximum  = 17.596350 (position = 14) 
    Sum      = 770.233930 
    Mean     = 17.505317 
    Variance = 0.000683 
    Std Dev  = 0.026126 
    CoV      = 0.001492 
---------------------------------------------------------------
```

En sachant que la vidéo doit être affichée à 15fps, nous avons un temps d'exécution de `~` 66ms pour chaque image. Nous sommes donc en dessous de la limite de 66.67ms pour afficher une image à 15fps.

On remarque que le temps d'exécution est très stable, avec un écart type de `~` 0.026ms.

=== Greyscale

```
-----------------------summary1.c -----------------------------
  Total of 44 values 
    Minimum  = 27.527860 (position = 25) 
    Maximum  = 27.737950 (position = 0) 
    Sum      = 1215.155190 
    Mean     = 27.617163 
    Variance = 0.001920 
    Std Dev  = 0.043822 
    CoV      = 0.001587 
---------------------------------------------------------------
```

Le traitement en niveaux de gris n'est pas très gourmand en temps. Nous sommes aussi en dessous de la limite de 66.67ms pour afficher une image à 15fps. 

En comparaison avec la tâche vidéo, la variation du temps d'exécution est l'égérement plus élevé, avec un écart type de `~` 0.043ms (`~` 1.5 fois plus élevé).

=== Convolution

```
-----------------------summary1.c -----------------------------
  Total of 44 values 
    Minimum  = 110.237930 (position = 15) 
    Maximum  = 111.755900 (position = 12) 
    Sum      = 4872.175690 
    Mean     = 110.731266 
    Variance = 0.093900 
    Std Dev  = 0.306431 
    CoV      = 0.002767 
---------------------------------------------------------------
```

La convolution est la tâche la plus gourmande en temps. Nous sommes en dessous de la limite de 66.67ms pour afficher une image à 15fps. Il ne serra pas possible d'ordonnancer cette tâche avec les autres tâches.

Un temps d'exécution si élevé est parfaitement compréhensible, car la convolution est une opération demandant plusieur calculs pour chaque pixel de l'image.

= Ordonnancement des tâches

// Afin que notre ensemble de tâches soit ordonnançable avec l’algorithme Rate Monotonic, quelles sont les périodes min. pour la vidéo (avec les différents traitements des données) en appliquant la condition d’ordonnançabilité proposée par Liu et Layland.

Pour que notre ensemble de tâches soit ordonnançable avec l’algorithme Rate Monotonic, nous devons respecter la condition d’ordonnançabilité proposée par Liu et Layland. 

Une condition suffisante d’ordonnançabilité proposée par Liu et Layland est :

$ sum_(i=1)^n C_i / P_i <= U_"lub"_"RM" (n) = n(2^(1/2) - 1) $

Avec ` C_i ` le temps d'exécution de la tâche i, ` P_i ` la période de la tâche i et ` U_lub_RM (n) ` la borne supérieure de l'utilisation du processeur pour ` n ` tâches.

_ Quelle observations faîtes-vous pour les différents traitements des données vidéos ? _

Comme indiqué précédemment, la tâche de convolution n'est pas ordonnançable avec les autres tâches. En effet, le temps d'exécution de la convolution est trop élevé pour être ordonnançable.


= Conclusion




// #pagebreak()

// math:
// $ (x² + 3) / 56 dot ln(2) = 42 "sec"  $

// pour les images :
// #image(./images/img.png, width: 23em)

// Pour deux images cote a cote :
// #align(center, table(columns: 2, stroke: none, [#image("plot/audio_task.svg", width: 18em)], [#image("plot/video_task.svg", width: 18em)]))
