import 'package:flutter_test/flutter_test.dart';
import 'package:orbita/data/models/spectrum_summary.dart';
import 'package:orbita/data/services/gemini_service.dart';
import 'package:google_generative_ai/google_generative_ai.dart';
import 'package:mockito/mockito.dart';

// Fake class to avoid code generation dependency for this test
class FakeGenerativeModel extends Fake implements GenerativeModel {
  String? lastPrompt;
  final String? fixedResponse;

  FakeGenerativeModel({this.fixedResponse});

  @override
  Future<GenerateContentResponse> generateContent(Iterable<Content> prompt, {List<SafetySetting>? safetySettings, GenerationConfig? generationConfig}) async {
    lastPrompt = prompt.map((e) => e.parts.whereType<TextPart>().map((t) => t.text).join()).join();
    if (fixedResponse != null) {
        return GenerateContentResponse([
            Candidate(
                Content.text(fixedResponse!),
                null,
                null,
                null,
                null,
            )
        ], null);
    }
    return GenerateContentResponse([], null);
  }
}

void main() {
  group('GeminiService Tests', () {
    late GeminiService service;
    late FakeGenerativeModel fakeModel;

    setUp(() {
      fakeModel = FakeGenerativeModel();
      service = GeminiService.withModel(fakeModel);
    });

    test('spectrumToPhilosophy sends correct prompt', () async {
      final responseText = "Mocked philosophy";
      fakeModel = FakeGenerativeModel(fixedResponse: responseText);
      service = GeminiService.withModel(fakeModel);

      final summary = SpectrumSummary(
        dominantFrequencies: [3.0, 5.0],
        chaosLevel: 0.8,
        density: 0.9,
      );

      final result = await service.spectrumToPhilosophy(summary);

      expect(result, responseText);
      expect(fakeModel.lastPrompt, contains('Dominant Frequencies: [3.0, 5.0]'));
      expect(fakeModel.lastPrompt, contains('Chaos Level (0-1): 0.8'));
      expect(fakeModel.lastPrompt, contains('Ink Density: 0.9'));
      expect(fakeModel.lastPrompt, contains('Output: A poetic, timeless interpretation in Turkish.'));
    });

    test('parseSpectrumJson parses valid JSON with double quotes', () {
      const jsonStr = '''
      {
        "core": [{"freq": 3.0, "amp": 50.0, "chaosFactor": 0.2}],
        "narrative": [{"freq": 10.0, "amp": 30.0, "chaosFactor": 0.3}],
        "nuance": [{"freq": 25.0, "amp": 10.0, "chaosFactor": 0.4}]
      }
      ''';

      final spectrum = service.parseSpectrumJson(jsonStr);

      expect(spectrum.coreLayer.length, 1);
      expect(spectrum.coreLayer.first.frequency, 3.0);
      expect(spectrum.coreLayer.first.amplitude, 50.0);
      expect(spectrum.coreLayer.first.chaosFactor, 0.2);
    });

    test('parseSpectrumJson parses JSON with single quotes', () {
      const jsonStr = '''
      {
        'core': [{'freq': 3.0, 'amp': 50.0, 'chaosFactor': 0.2}],
        'narrative': [],
        'nuance': []
      }
      ''';

      final spectrum = service.parseSpectrumJson(jsonStr);

      expect(spectrum.coreLayer.length, 1);
      expect(spectrum.coreLayer.first.frequency, 3.0);
    });

    test('parseSpectrumJson handles markdown code blocks', () {
      const jsonStr = '''
      ```json
      {
        "core": [{"freq": 3.0, "amp": 50.0, "chaosFactor": 0.2}],
        "narrative": [],
        "nuance": []
      }
      ```
      ''';

      final spectrum = service.parseSpectrumJson(jsonStr);

      expect(spectrum.coreLayer.length, 1);
    });
  });
}
