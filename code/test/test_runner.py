import subprocess
import os
import sys


NACHOS_CMD = "../build/nachos-step2"


USER_PROG = "../build/console_test" 

INPUT_FILE = "input.txt"

def run_test():
    # Vérification que l'exécutable utilisateur existe
    if not os.path.exists(USER_PROG):
        print(f"ERREUR : Le programme '{USER_PROG}' n'existe pas.")
        print(" Avez-vous lancé la commande 'make' dans le dossier test/ ?")
        return

    # Vérification que NachOS existe
    if not os.path.exists(NACHOS_CMD):
        print(f" ERREUR : L'exécutable NachOS '{NACHOS_CMD}' est introuvable.")
        print(" Avez-vous lancé la commande 'make' dans le dossier build/ ?")
        return

    # Création du fichier d'entrée par défaut si absent
    if not os.path.exists(INPUT_FILE):
        print(f"  Création d'un fichier {INPUT_FILE} par défaut...")
        with open(INPUT_FILE, "w") as f:
            f.write("TestAutomatique")
    
    # Lecture des données attendues pour comparaison
    with open(INPUT_FILE, 'rb') as f:
        input_data = f.read()
        expected_str = input_data.decode('utf-8').strip()

    print(f" Lancement du test via : {NACHOS_CMD}")
    print(f" Programme utilisateur : {USER_PROG}")
    #print(f" Donnée injectée : '{expected_str}'")

    cmd = [NACHOS_CMD, '-x', USER_PROG]

    try:
        process = subprocess.Popen(
            cmd,
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE
        )

        # Timeout de 3 secondes pour détecter les blocages
        stdout, stderr = process.communicate(input=input_data, timeout=20)

    except subprocess.TimeoutExpired:
        process.kill()
        print("\n ÉCHEC CRITIQUE : Timeout !")
        print("Le programme a mis trop de temps. Probablement un Deadlock ou une boucle infinie.")
        return

    # Analyse de la sortie
    output_text = stdout.decode('utf-8', errors='ignore')
    
    # Balises de repérage (doivent correspondre à votre C)
    MARKER_START = "<<<START_TEST>>>"
    MARKER_END = "<<<END_TEST>>>"

    if MARKER_START in output_text and MARKER_END in output_text:
        start_idx = output_text.find(MARKER_START) + len(MARKER_START)
        end_idx = output_text.find(MARKER_END)
        actual_output = output_text[start_idx:end_idx].strip()
        
        #print("\n--- RÉSULTATS ---")
        #print(f"Attendu : '{expected_str}'")
        #print(f"Reçu    : '{actual_output}'")
        
        if expected_str in actual_output:
            print("\n TEST PASSÉ : SUCCÈS")
        else:
            print("\n TEST ÉCHOUÉ : Contenu incorrect.")
    else:
        print("\n TEST ÉCHOUÉ : Balises non trouvées.")
        print("Vérifiez que console_test.c contient bien PutString(\"<<<START_TEST>>>\").")
        print("--- Sortie brute (début) ---")
        print(output_text[:300])

if __name__ == "__main__":
    run_test()