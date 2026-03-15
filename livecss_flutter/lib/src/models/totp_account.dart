class TotpAccount {
  const TotpAccount({
    required this.id,
    required this.issuer,
    required this.label,
    required this.secret,
    this.algorithm = 'SHA1',
    this.digits = 6,
    this.period = 30,
  });

  final String id;
  final String issuer;
  final String label;
  final String secret;
  final String algorithm;
  final int digits;
  final int period;

  TotpAccount copyWith({
    String? id,
    String? issuer,
    String? label,
    String? secret,
    String? algorithm,
    int? digits,
    int? period,
  }) {
    return TotpAccount(
      id: id ?? this.id,
      issuer: issuer ?? this.issuer,
      label: label ?? this.label,
      secret: secret ?? this.secret,
      algorithm: algorithm ?? this.algorithm,
      digits: digits ?? this.digits,
      period: period ?? this.period,
    );
  }

  Map<String, dynamic> toJson() => {
        'id': id,
        'issuer': issuer,
        'label': label,
        'secret': secret,
        'algorithm': algorithm,
        'digits': digits,
        'period': period,
      };

  factory TotpAccount.fromJson(Map<String, dynamic> json) {
    return TotpAccount(
      id: (json['id'] ?? '') as String,
      issuer: (json['issuer'] ?? '') as String,
      label: (json['label'] ?? '') as String,
      secret: (json['secret'] ?? '') as String,
      algorithm: (json['algorithm'] ?? 'SHA1') as String,
      digits: (json['digits'] as num? ?? 6).toInt(),
      period: (json['period'] as num? ?? 30).toInt(),
    );
  }
}

class SecuritySettings {
  const SecuritySettings({
    this.biometricsEnabled = false,
    this.obscureCodes = false,
    this.autoLockSeconds = 30,
  });

  final bool biometricsEnabled;
  final bool obscureCodes;
  final int autoLockSeconds;

  SecuritySettings copyWith({
    bool? biometricsEnabled,
    bool? obscureCodes,
    int? autoLockSeconds,
  }) {
    return SecuritySettings(
      biometricsEnabled: biometricsEnabled ?? this.biometricsEnabled,
      obscureCodes: obscureCodes ?? this.obscureCodes,
      autoLockSeconds: autoLockSeconds ?? this.autoLockSeconds,
    );
  }

  Map<String, dynamic> toJson() => {
        'biometrics_enabled': biometricsEnabled,
        'obscure_codes': obscureCodes,
        'auto_lock_seconds': autoLockSeconds,
      };

  factory SecuritySettings.fromJson(Map<String, dynamic> json) {
    return SecuritySettings(
      biometricsEnabled: json['biometrics_enabled'] == true,
      obscureCodes: json['obscure_codes'] == true,
      autoLockSeconds: (json['auto_lock_seconds'] as num? ?? 30).toInt(),
    );
  }
}
