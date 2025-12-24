import 'package:flutter_test/flutter_test.dart';
import 'package:orbita/ui/painters/heptapod_painter.dart';
import 'package:orbita/data/models/heptapod_spectrum.dart';
import 'package:orbita/data/models/wave_layer.dart';
import 'package:flutter/material.dart';

void main() {
  test('HeptapodPainter should instantiate and not throw', () {
    final spectrum = HeptapodSpectrum(
      coreLayer: [WaveLayer(frequency: 4, amplitude: 0.5, chaosFactor: 0.1)],
      narrativeLayer: [WaveLayer(frequency: 10, amplitude: 0.3)],
      nuanceLayer: [WaveLayer(frequency: 30, amplitude: 0.1, chaosFactor: 0.8)],
    );

    final painter = HeptapodPainter(spectrum: spectrum, seed: 123);

    // We cannot easily test paint() without a real canvas/context in a unit test
    // without mocking significantly, but we can check if it compiles and initializes.
    expect(painter, isNotNull);
    expect(painter.spectrum, equals(spectrum));
  });
}
