import 'package:flutter_test/flutter_test.dart';
import 'package:orbita/data/models/heptapod_spectrum.dart';
import 'package:orbita/data/services/gemini_service.dart';
import 'package:google_generative_ai/google_generative_ai.dart';
import 'package:mockito/mockito.dart';

// Fake class to avoid code generation dependency for this test
class FakeGenerativeModel extends Fake implements GenerativeModel {
  @override
  Future<GenerateContentResponse> generateContent(Iterable<Content> prompt, {List<SafetySetting>? safetySettings, GenerationConfig? generationConfig}) async {
    // This is just a stub to allow instantiation.
    // If we were testing textToSpectrum, we would return a fake response here.
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
