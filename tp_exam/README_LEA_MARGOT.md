## Message de rendu

Bonjour, nous vous prions de trouver dans ce repo notre travail concernant le projet secos-ng

Nous avons pris la liberté de modifier le noyau donné, en particulier les fichiers 
- idt.s (modification mineure dans idt_common pour gestion du switch) 
- intr.c (modification majeure dans intr_hdlr afin de traiter irq0 et int 80)
- linker.lds (ajouts significatifs dans le code afin de configurer notre memoire physique)

---

Si vous avez des questions complémentaires, n'hésitez pas à nous joindre par mail.

## Organisation

Dans le dossier configurations_files sont présents nos fichiers .c et .h, utilisés dans tp.c, 
qui contiennent nos codes pour les configurations de chaque étape (segmentation, pagination, etc.)

Dans le dossier autre sont présents des fichiers non essentiels au projet, uniquement
utilisés pour des améliorations d'affichages, etc.

Le fichier task.h permet de sauvegarder des proprietés propres a chaque user grâce à une struct

Le dossier docs contient les schémas utiles à la compréhension de nos choix. 