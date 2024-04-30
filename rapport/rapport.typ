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

Nous examinerons et analyserons également les différents temps d’exécutions des tâches afin d’étu-
dier et d’optimiser les performances temps réel et l’implémentation de celles-ci.

= Tâche Audio



= Tâche Vidéo



== Amélioration possible



= Conclusion




// #pagebreak()

// math:
// $ (x² + 3) / 56 dot ln(2) = 42 "sec"  $

// pour les images :
// #image(./images/img.png, width: 23em)

// Pour deux images cote a cote :
// #align(center, table(columns: 2, stroke: none, [#image("plot/audio_task.svg", width: 18em)], [#image("plot/video_task.svg", width: 18em)]))
