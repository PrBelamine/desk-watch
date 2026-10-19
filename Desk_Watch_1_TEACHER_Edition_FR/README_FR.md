# Desk Watch 1 — TEACHER Edition

Cette édition ajoute l’application **EXAMEN** à toutes les fonctions de MELODY Edition. Les commentaires de ce fichier `.ino` sont en français ; les quatre langues de l’interface restent disponibles dans les paramètres. Les fichiers MELODY sont conservés séparément.

## Démarrer un examen

Après l’introduction, l’appareil ouvre le menu sur **Favori, entrée 1/11**, quelle que soit la position initiale de la molette. Le menu reste sur cette entrée jusqu’au premier mouvement de la molette. Choisir **Examen**, juste après Favori, puis cliquer pour ouvrir sa préparation.

1. Tourner la molette pour choisir la durée, de **1 à 240 minutes**, puis cliquer.
2. Choisir le nombre de copies attendu, de **1 à 99**, puis cliquer.
3. Vérifier le récapitulatif et cliquer pour démarrer.

Les valeurs initiales proposées sont **35 minutes et 24 copies**.

```text
EXAMEN     34:59
Copies     00/24
```

Chaque appui court ajoute immédiatement une copie. Deux appuis rapides comptent deux copies ; un double clic ne remet jamais le compteur à zéro. Le total ne dépasse pas le nombre attendu.

## Commandes pendant la collecte

Faire un **appui long de 0,8 à 2,5 secondes**, puis sélectionner une commande à la molette et cliquer :

- **Retour** : revenir à l’écran de collecte.
- **Retirer 1 copie** : corriger un appui en trop, sans descendre sous zéro.
- **Pause / Reprendre** : suspendre ou relancer le décompte ; la collecte reste utilisable en pause.
- **+5 min** : ajouter cinq minutes, dans la limite de quatre heures restantes. Si le temps était écoulé, le décompte repart pour cinq minutes.
- **Terminer** : demande une confirmation. Clic = afficher le récapitulatif final ; double clic = annuler. Un nouveau clic sur le récapitulatif revient à la préparation du prochain examen.
- **Menu principal** : utiliser une autre app. Le décompte continue en arrière-plan. Revenir par l’entrée Examen pour reprendre la collecte.

Un appui long dans les commandes revient à la collecte. Un appui long pendant la préparation ouvre le menu principal. La molette seule ne quitte pas l’écran de collecte.

## Fin du temps et fin de collecte

À zéro, deux bips courts retentissent une seule fois et l’écran indique **TEMPS ECOULE**. Le compteur reste disponible pour les dernières copies.

Quand toutes les copies sont rendues, l’écran devient vert et affiche **COPIES RENDUES**, avec le total. Cela ne termine pas automatiquement la session ; utiliser la commande Terminer.

À la fin du temps ou lorsque toutes les copies sont rendues, la première ligne alterne toutes les deux secondes entre le message de fin et la **durée réellement écoulée, hors pauses**, sous la forme `TOTAL      35:00`. Le nombre de copies reste visible en deuxième ligne. Cette durée se fige à la dernière copie rendue ou à l’expiration du temps ; la collecte après expiration n’y ajoute pas de temps.

La commande Terminer affiche cette même durée dans un récapitulatif stable avec le total des copies, puis un clic permet de préparer l’examen suivant. Les prolongations ajoutent du temps disponible sans augmenter instantanément la durée réellement écoulée.

La veille automatique est suspendue pendant une session, y compris en pause et après la fin du temps. L’appui très long de plus de 2,5 secondes conserve sa fonction de veille manuelle. Le temps restant et la collecte sont conservés en RAM pendant la navigation et la veille, pas après une coupure d’alimentation ou une réinitialisation.

## Matériel et validation

Même matériel et mêmes bibliothèques que MELODY Edition : Arduino UNO, écran Grove LCD RGB I2C, bouton D4, molette A1 et buzzer D3. Le mode EXAMEN fonctionne sans RTC ; le RTC reste utilisé par l’horloge et le réveil.

Compilation Arduino UNO vérifiée. Les tests de logique dérivés du code couvrent le scénario 35 min / 24 copies, les appuis rapprochés, la pause/reprise, la correction, la prolongation, la collecte après expiration, la confirmation de fin et le débordement de `millis()`. L’affichage, le bouton et le son restent à vérifier sur l’appareil réel.
