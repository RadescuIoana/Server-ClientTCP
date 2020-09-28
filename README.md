Radescu Ioana

# Server

	Pentru inceput, se creeaza socketii pentru conexiunile UDP si TCP, se realizeaza conectarea cu o adresa 
	pentru fiecare si se asculta pe socket-ul TCP. Se adauga apoi socketii la setul de descriptori de fisier.

	Se itereaza pin toti sochetii, verificandu-se daca apartin setului de descriptori. In caz afirmativ se 
	verifica daca socketul curent este TCP, UDP sau STDIN.

	Pentru un socket TCP, se verifica daca nu cumva clientul este deja conectat la server, in caz afirmativ, 
	se reconecteaza pe un nou socket, actualizandu-i-se datele. In caz contrar, se introduce noul client in 
	vectorul de client si se realizeaza conexiunea cu serverul.

	Pentru un socket UDP, se citesc mesajele primite, se parseaza si se aduc in formatul dorit si se trimit 
	clientilor abonati in functie de topicul fiecarui mesaj.
	Pentru stdin, se verifica daca comanda primita este EXIT. In caz afirmativ se inchid socketii UDP, TCP 
	si se inchide programul.

	Daca se ajunge in cazul final, inseamna ca un client TCP incearca sa contacteze serverul. Acesta putand 
	sa realizeze o cerere de subscribe la un topic sau unsubscribe de la unul. 
	Se trateaza fiecare caz, astfel, pentru subscribe se actualizeaza lista de topicuri atasata clientului 
	respectiv, iar pentru unsubscribe, se modifica topicul deja existent in lista clientului, acesta avand 
	acum campul de unsubscribe setat la 1.
	Exista si situatia in are serverul nu primeste un mesaj de la clientul TCP, caz in care se va realiza 
	deconectarea acestuia prin setarea campului status al clientului la 0.

# clientTCP

	Se realizeaza conexiunea clientului la server si se adauga socketul in lista de descriptoti de fisier, 
	alaturi de socket-ul 0(stdin).
	
	Se itereaza prin setul de descriptori de fisier. 
	Pentru stdin se citeste comanda si se trimite la server.
	Pentru server se primeste mesajul trimis de acesta si se afiseaza.

