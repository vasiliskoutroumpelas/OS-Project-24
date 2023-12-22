##################################
# Vasileios Koutroumpelas, 1093397
# Filippos Minopetros, 1093431
##################################

#!/bin/bash

file="Businesses.csv"
clear
while true; do
    # Εκτύπωση μενού
    echo "[1] Επιλογή αρχείου επιχειρήσεων"
    echo "[2] Προβολή στοιχείων επιχείρησης"
    echo "[3] Αλλαγή στοιχείου επιχείρησης"
    echo "[4] Προβολή αρχείου"
    echo "[5] Αποθήκευση αρχείου"
    echo "[6] Έξοδος"
    
    # Επιλογή χρήστη
    echo "Επιλέξτε μια λειτουργία (1-6): "
    read choice

    case $choice in
        1)
            echo "Δώστε το path του αρχείου επιχειρήσεων"
            read file
            clear
            if [ ! -f "$file" ]; then
                echo "Το αρχείο που δώσατε δεν υπάρχει"
            fi
            ;;
        2)
            echo "Δώστε τον κώδικό της επιχείρησης"
            read business_code
            clear
            awk -F, -v business_code="$business_code" '$1==business_code {print "Κωδικός:", $1, "\nΌνομα:", $2, "\nΟδός:", $3, "\nΠόλη:", $4, "\nΤαχ. Κώδικας:", $5, "\nΓεωγρ. Μήκος:", $6, "\nΓεωγρ. Πλάτος:", $7}' "$file"
            echo "Πατήστε enter για να συνεχίσετε"
            read
            clear
            ;;
        3)
            echo "Δώστε τον κώδικό της επιχείρησης για αλλαγή στοιχείου"
            read business_code
            
            # Convert field name to order of its apperance in the columns of the csv file 
            fieldNumber=0
            while [[ "$fieldNumber" == 0 ]]; do
                echo "Δώστε το όνομα του στοιχείου προς αλλαγή"
                echo "Διαθέσιμες επιλογές: ID,BusinessName,AddressLine2,AddressLine3,PostCode,Longitude,Latitude"
                read field
                if [[ "$field" == "ID" ]]; then
                    fieldNumber=1
                elif [[ "$field" == "BusinessName" ]]; then
                    fieldNumber=2
                elif [[ "$field" == "AddressLine2" ]]; then
                    fieldNumber=3
                elif [[ "$field" == "AddressLine3" ]]; then
                    fieldNumber=4
                elif [[ "$field" == "PostCode" ]]; then
                    fieldNumber=5
                elif [[ "$field" == "Longitude" ]]; then
                    fieldNumber=6
                elif [[ "$field" == "Latitude" ]]; then
                    fieldNumber=7
                else
                    continue
                fi
            done

            echo "Δώστε την νεα τιμή"
            read new_value

            # Print old value
            awk -F, -v business_code="$business_code" -v fieldNumber="$fieldNumber" '$1==business_code {print "Παλαιά τιμή:", $fieldNumber}' "$file"
                        
            # Αν το προσωρινό αρχείο δεν υπάρχει τότε δημιουργείται
            if [ ! -f ".temp" ]; then
                cp "$file" ".temp"
                
                # Make the temporary file the one to be working on
                file=".temp"
            fi

            sed -i "/^$business_code/s/[^,]*/$new_value/$fieldNumber" "$file"    

            # Αν η αλλαγή έγινε στο πεδίο ID αλλάζουμε το business_code ώστε να εμφανιστεί το καινούργιο
            if [ $fieldNumber == 1 ]; then
                business_code="$new_value"
            fi               

            # Print new value
            awk -F, -v business_code="$business_code" -v fieldNumber="$fieldNumber" '$1==business_code {print "Νεα τιμή:", $fieldNumber}' "$file"

            echo "Πατήστε enter για να συνεχίσετε"
            read
            clear
            ;;
        4)
            clear
            more "$file"
            clear
            ;;
        5)
            echo "Δώστε το path του αρχείου για αποθήκευση (πατήστε Enter για προεπιλεγμένο όνομα 'Businesses.csv')"
            read path
            if [ -z "$path" ]; then
                path="Businesses.csv"
            fi

            # Αν το αρχείο έχει υποστεί επεξεργασία (δηλ. είναι πλεον το .tmp) τότε αποθηκεύεται αλλιώς δεν γίνεται τίποτα
            if [ "$file" == ".temp" ]; then
                cp "$file" "$path"
            fi
            echo "Πατήστε enter για να συνεχίσετε"
            read
            clear
            ;;
        6)
            # Έξοδος
            # Remove the temporary file
            rm ".temp"
            clear
            echo "Τερματισμός του προγράμματος."
            exit 0
            ;;
        *)
            # Μη έγκυρη επιλογή
            clear
            echo "Μη έγκυρη επιλογή. Παρακαλώ προσπαθήστε ξανά."
            ;;
    esac
done