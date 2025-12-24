import 'dart:io';
import 'package:flutter_riverpod/flutter_riverpod.dart';
import '../data/services/gemini_service.dart';
import '../data/services/analysis_service.dart';
import '../data/models/heptapod_spectrum.dart';
import '../data/models/spectrum_summary.dart';

// Providers for Services
// Note: In a real app, API Key should be secured.
// Ideally passed via environment variable or user input.
// For now we will use a placeholder or expect it to be set.
final geminiApiKeyProvider = Provider<String>((ref) => 'AIzaSyAONEDj8aPeU4P7C1nm7HcVV8Xob1oAZD4');

final geminiServiceProvider = Provider<GeminiService>((ref) {
  final apiKey = ref.watch(geminiApiKeyProvider);
  return GeminiService(apiKey);
});

final analysisServiceProvider = Provider<AnalysisService>((ref) {
  return AnalysisService();
});

// State definitions
class HeptapodState {
  final HeptapodSpectrum? generatedSpectrum;
  final String? inputText;
  final File? selectedImage;
  final SpectrumSummary? analysisResult;
  final String? interpretation;
  final bool isProcessing;
  final String? error;

  HeptapodState({
    this.generatedSpectrum,
    this.inputText,
    this.selectedImage,
    this.analysisResult,
    this.interpretation,
    this.isProcessing = false,
    this.error,
  });

  HeptapodState copyWith({
    HeptapodSpectrum? generatedSpectrum,
    String? inputText,
    File? selectedImage,
    SpectrumSummary? analysisResult,
    String? interpretation,
    bool? isProcessing,
    String? error,
  }) {
    return HeptapodState(
      generatedSpectrum: generatedSpectrum ?? this.generatedSpectrum,
      inputText: inputText ?? this.inputText,
      selectedImage: selectedImage ?? this.selectedImage,
      analysisResult: analysisResult ?? this.analysisResult,
      interpretation: interpretation ?? this.interpretation,
      isProcessing: isProcessing ?? this.isProcessing,
      error: error ?? this.error,
    );
  }
}

class HeptapodNotifier extends StateNotifier<HeptapodState> {
  final GeminiService _geminiService;
  final AnalysisService _analysisService;

  HeptapodNotifier(this._geminiService, this._analysisService)
      : super(HeptapodState());

  Future<void> materialize(String text) async {
    if (text.isEmpty) return;

    state = state.copyWith(isProcessing: true, error: null, inputText: text);

    try {
      final spectrum = await _geminiService.textToSpectrum(text);
      state = state.copyWith(
        generatedSpectrum: spectrum,
        isProcessing: false,
      );
    } catch (e) {
      state = state.copyWith(
        isProcessing: false,
        error: e.toString(),
      );
    }
  }

  Future<void> interpret(File image) async {
    // Clear previous interpretation and set processing state
    state = state.copyWith(
      isProcessing: true,
      error: null,
      selectedImage: image,
      interpretation: null,
      analysisResult: null,
    );

    try {
      // 1. Analyze Image
      final summary = await _analysisService.analyzeImage(image);
      state = state.copyWith(analysisResult: summary);

      // 2. Interpret Summary
      final poem = await _geminiService.spectrumToPhilosophy(summary);
      state = state.copyWith(
        interpretation: poem,
        isProcessing: false,
      );
    } catch (e) {
      state = state.copyWith(
        isProcessing: false,
        error: e.toString(),
      );
    }
  }

  void clearError() {
    state = state.copyWith(error: null);
  }
}

final heptapodProvider = StateNotifierProvider<HeptapodNotifier, HeptapodState>((ref) {
  final gemini = ref.watch(geminiServiceProvider);
  final analysis = ref.watch(analysisServiceProvider);
  return HeptapodNotifier(gemini, analysis);
});
