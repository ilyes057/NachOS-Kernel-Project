# NachOS Kernel Development - M1 UGA

## 📝 Présentation
Projet de développement système réalisé dans le cadre du Master 1 Informatique à l'Université Grenoble Alpes. L'objectif est l'extension d'un noyau fonctionnel (NachOS) sur architecture MIPS.

## 🛠️ Réalisations Techniques

### 1. Multithreading & Synchronisation
* Implémentation des threads utilisateurs.
* Gestion de la synchronisation via **Sémaphores**, **Locks** et **Variables de Condition**.

### 2. Gestion de la Mémoire Virtuelle
* Implémentation de la pagination et de l'isolation des espaces d'adressage.
* Gestion logicielle de la **TLB** et des exceptions de segmentation.

### 3. Couche Réseau
* Développement d'appels système pour la communication par **sockets**.
* Support du transfert de données entre instances NachOS.

## 📌 État du Projet & Certification
* **Branche stable :** `maria2`
* **Version finale :** Tag `final`
* **ID du Commit Parent :** `0e82ed40`

## 💻 Stack Technique
* **Langages :** C / C++
* **Outils :** GDB, Valgrind, Make, Git