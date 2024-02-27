import React from 'react';
import { StatusBar } from "expo-status-bar";
import { StyleSheet, Text, View, TouchableOpacity } from "react-native";
import axios from "axios";

const url = "http://192.168.1.200/";

export default function App() {
    const [status, setStatus] = React.useState("Clique em atualizar");
    React.useEffect(() => {atualizaStatus()}, []);

    function atualizaStatus() {
        axios
            .get(url)
            .then((response) => {
                setStatus(response.data.message);
            })
            .catch((error) => {
                alert("Um erro ocorreu: " + error);
            });
    }

    function enviaComando(comando) {
        axios
            .get(url + comando)
            .then((response) => {
                if (!response.data.success) {
                    alert(response.data.message);
                }
            })
            .catch((error) => {
                alert("Um erro ocorreu: " + error);
            });
            atualizaStatus();
    }

    return (
        <View style={styles.container}>
            <Text style={styles.text}>Status: {status}</Text>
            <TouchableOpacity
                style={styles.greenButton}
                onPress={() => atualizaStatus()}
            >
                <Text>Atualizar</Text>
            </TouchableOpacity>
            <TouchableOpacity
                style={styles.button}
                onPress={() => enviaComando("lavagemRapida")}
            >
                <Text>Lavagem rápida</Text>
            </TouchableOpacity>
            <TouchableOpacity
                style={styles.button}
                onPress={() => enviaComando("apenasEsvaziarEnxaguarECentrifugar")}
            >
                <Text>Esvaziar, enxaguar e centrifugar</Text>
            </TouchableOpacity>
            <TouchableOpacity
                style={styles.button}
                onPress={() => enviaComando("apenasEnxaguarECentrifugar")}
            >
                <Text>Enxaguar e centrifugar</Text>
            </TouchableOpacity>
            <TouchableOpacity
                style={styles.button}
                onPress={() => enviaComando("apenasCentrifugar")}
            >
                <Text>Centrifugar</Text>
            </TouchableOpacity>
            <TouchableOpacity
                style={styles.button}
                onPress={() => enviaComando("apenasEsvaziar")}
            >
                <Text>Esvaziar</Text>
            </TouchableOpacity>
            <TouchableOpacity
                style={styles.redButton}
                onPress={() => enviaComando("cancelarLavagem")}
            >
                <Text>Cancelar lavagem</Text>
            </TouchableOpacity>
            <StatusBar style="auto" />
        </View>
    );
}

const styles = StyleSheet.create({
    container: {
        flex: 1,
        backgroundColor: "#fff",
        alignItems: "center",
        justifyContent: "center",
    },
    text: {
        fontSize: 20,
        marginBottom: 20,
    },
    button: {
        backgroundColor: "#F8F8E5",
        marginBottom: 30,
        marginRight: 10,
        padding: 10,
        alignItems: "center",
        width: "100%",
    },
    greenButton: {
        backgroundColor: "#B5DBCC",
        marginBottom: 30,
        marginRight: 10,
        padding: 10,
        alignItems: "center",
        width: "100%",
    },
    redButton: {
        backgroundColor: "#F59E87",
        marginBottom: 30,
        marginRight: 10,
        padding: 10,
        alignItems: "center",
        width: "100%",
    },
    buttonText: {
        color: "#ffffff",
        fontSize: 15,
    },
});
