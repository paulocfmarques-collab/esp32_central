# Changelog

Todas as mudanças relevantes neste projeto serão documentadas neste arquivo.

O formato segue boas práticas inspiradas em *Keep a Changelog*, com foco em clareza, rastreabilidade e apresentação profissional.

## [Unreleased]

### Added
- README reorganizado com visão geral, arquitetura, comandos e estrutura do repositório.
- Seção dedicada à documentação do repositório e aos principais módulos do firmware.
- Referência explícita ao histórico profissional de mudanças.

### Changed
- Documentação principal refinada para melhorar legibilidade, consistência e navegação.
- Descrição do projeto ajustada para refletir de forma mais objetiva a proposta da central ESP32.

## [1.0.0] - 2026-10-06

### Added
- Firmware inicial da central ESP32 com interface touchscreen.
- Modo local para diagnósticos e comandos internos.
- Modo remoto para comunicação UDP com até 4 escravos.
- Portal web de configuração em `192.168.4.1`.
- Sincronização NTP e persistência de configuração.
- Suporte a OTA.

### Changed
- Estrutura modularizada em componentes de display, touch, comunicação, Wi‑Fi e interface.
- Organização do projeto para facilitar manutenção e evolução.
