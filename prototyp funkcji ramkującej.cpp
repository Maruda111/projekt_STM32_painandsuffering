#include <iostream>
#include <vector>
#include <cstdint>
#include <deque>

// Stale protokolu
constexpr uint8_t START = 0x02; // Znak otwierajacy
constexpr uint8_t END = 0x03;   // Znak zamykajacy
constexpr uint8_t ESC = 0x1B;   // Znak ucieczki

// Klasa bufora kolowego
class CircularBuffer {
public:
    CircularBuffer(size_t capacity) : capacity(capacity) {}

    void write(uint8_t byte) {
        if (buffer.size() == capacity) {
            buffer.pop_front(); // Usuwamy najstarszy element
        }
        buffer.push_back(byte);
    }

    std::vector<uint8_t> readAll() {
        std::vector<uint8_t> data(buffer.begin(), buffer.end());
        buffer.clear();
        return data;
    }

    size_t size() const {
        return buffer.size();
    }

private:
    size_t capacity;
    std::deque<uint8_t> buffer;
};

// Prototypy funkcji
uint8_t calculateCRC(const std::vector<uint8_t>& data);
std::vector<uint8_t> buildFrame(uint8_t addr, uint8_t cmd, const std::vector<uint8_t>& data);
bool verifyFrame(const std::vector<uint8_t>& frame);
std::vector<uint8_t> maskData(const std::vector<uint8_t>& data);
std::vector<uint8_t> unmaskData(const std::vector<uint8_t>& data);
std::vector<uint8_t> processStream(CircularBuffer& buffer);

int main() {
    // Przyklad: Budowa ramki
    uint8_t address = 0x05;
    uint8_t command = 0x10;
    std::vector<uint8_t> data = {0xDE, 0xAD, 0x02, 0x03, 0x1B};

    // Budowanie ramki
    std::vector<uint8_t> frame = buildFrame(address, command, data);

    // Wyswietlanie ramki
    std::cout << "Zbudowana ramka: ";
    for (uint8_t byte : frame) {
        std::cout << std::hex << (int)byte << " ";
    }
    std::cout << std::endl;

    // Przyklad przetwarzania danych przez bufor kolowy
    CircularBuffer buffer(200); // Bufor o pojemnosci 200bajtow

    // Symulacja strumienia danych
    std::vector<uint8_t> stream = {START, 0x05, 0x10, 0xDE, START, 0x05, 0x10, 0xAD, 0x1B, 0x02, 0x03, 0x0A, END};
    for (uint8_t byte : stream) {
        buffer.write(byte);
    }

    // Przetwarzanie danych z bufora kolowego
    std::vector<uint8_t> result = processStream(buffer);

    if (!result.empty()) {
        std::cout << "Poprawna ramka znaleziona w buforze: ";
        for (uint8_t byte : result) {
            std::cout << std::hex << (int)byte << " ";
        }
        std::cout << std::endl;
    } else {
        std::cout << "Nie znaleziono poprawnej ramki w buforze." << std::endl;
    }

    return 0;
}

// Funkcja obliczajaca CRC-8
uint8_t calculateCRC(const std::vector<uint8_t>& data) {
    uint8_t crc = 0x00;
    const uint8_t polynomial = 0x07;

    for (uint8_t byte : data) {
        crc ^= byte;
        for (int i = 0; i < 8; ++i) {
            if (crc & 0x80) {
                crc = (crc << 1) ^ polynomial;
            } else {
                crc <<= 1;
            }
        }
    }

    return crc;
}

// Funkcja maskujaca dane
std::vector<uint8_t> maskData(const std::vector<uint8_t>& data) {
    std::vector<uint8_t> maskedData;
    for (uint8_t byte : data) {
        if (byte == START || byte == END || byte == ESC) {
            maskedData.push_back(ESC);
        }
        maskedData.push_back(byte);
    }
    return maskedData;
}

// Funkcja demaskujaca dane
std::vector<uint8_t> unmaskData(const std::vector<uint8_t>& data) {
    std::vector<uint8_t> unmaskedData;
    bool escapeNext = false;
    for (uint8_t byte : data) {
        if (escapeNext) {
            unmaskedData.push_back(byte);
            escapeNext = false;
        } else if (byte == ESC) {
            escapeNext = true;
        } else {
            unmaskedData.push_back(byte);
        }
    }
    return unmaskedData;
}

// Funkcja budujaca ramke
std::vector<uint8_t> buildFrame(uint8_t addr, uint8_t cmd, const std::vector<uint8_t>& data) {
    std::vector<uint8_t> frame;
    frame.push_back(START);       // Znak otwierajacy
    frame.push_back(addr);        // Adres odbiorcy
    frame.push_back(cmd);         // Kod polecenia

    // Maskowanie danych
    std::vector<uint8_t> maskedData = maskData(data);
    frame.insert(frame.end(), maskedData.begin(), maskedData.end());

    // Obliczanie sumy kontrolnej
    std::vector<uint8_t> crcData = {addr, cmd};
    crcData.insert(crcData.end(), data.begin(), data.end());
    uint8_t crc = calculateCRC(crcData);
    frame.push_back(crc);         // Suma kontrolna

    frame.push_back(END);         // Znak zamykajacy
    return frame;
}

// Funkcja weryfikujaca poprawnosc ramki
bool verifyFrame(const std::vector<uint8_t>& frame) {
    if (frame.size() < 5 || frame.front() != START || frame.back() != END) {
        return false; // Nieprawidlowy format ramki
    }

    // Ekstrakcja danych
    uint8_t addr = frame[1];
    uint8_t cmd = frame[2];
    std::vector<uint8_t> rawData(frame.begin() + 3, frame.end() - 2);

    // Demaskowanie danych
    std::vector<uint8_t> data = unmaskData(rawData);

    uint8_t receivedCRC = frame[frame.size() - 2];

    // Obliczanie sumy kontrolnej
    std::vector<uint8_t> crcData = {addr, cmd};
    crcData.insert(crcData.end(), data.begin(), data.end());
    uint8_t calculatedCRC = calculateCRC(crcData);

    return receivedCRC == calculatedCRC;
}

// Funkcja przetwarzajaca strumien danych z bufora kolowego
std::vector<uint8_t> processStream(CircularBuffer& buffer) {
    std::vector<uint8_t> data = buffer.readAll();
    std::vector<uint8_t> frame;
    bool inFrame = false;

    for (uint8_t byte : data) {
        if (byte == START) {
            if (inFrame) {
                // Napotkano nowy START w trakcie trwajacej ramki - odrzucamy biezace dane
                frame.clear();
            }
            inFrame = true; // Rozpoczynamy nowa ramke
            frame.push_back(byte);
        } else if (byte == END) {
            if (inFrame) {
                frame.push_back(byte);
                if (verifyFrame(frame)) {
                    return frame; // Zwracamy poprawna ramke
                }
                // Jesli ramka jest niepoprawna, czyszcimy bufor
                frame.clear();
                inFrame = false;
            }
        } else {
            if (inFrame) {
                frame.push_back(byte);
            }
        }
    }

    return {}; // Nie znaleziono poprawnej ramki
}
