def valorAscendente(dictionary):
    lista = list(dictionary.values())
    listaOrdenada = sorted(lista)
    for x in listaOrdenada:
        print(x)

def comparador(dict1,dict2):
    for x in dict1:
        if x in dict2:
            if dict1[x] == dict2[x]:
                print(f"La llave {x} esta en el segundo diccionario y su valor es {dict1[x]}")
            else:
                print(f"La llave {x} no tiene el mismo valor que el primer diccionario")
        else:
            print(f"La llave {x} no esta en el segundo diccionario")
    for x in dict2:
        if x in dict1:
            None
        else:
            print(f"La llave {x} con valor {dict2[x]} no esta en el primer diccionario")

def mezclador(dict1,dict2):
    diccionarioFinal = dict2
    for x in dict1:
            diccionarioFinal[x] = dict1[x]
    print(diccionarioFinal)