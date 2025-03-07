// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import BraveShared
import DataImporter
import Foundation
import MobileCoreServices
import PassKit
import Shared
import UniformTypeIdentifiers
import WebKit
import os.log

struct MIMEType {
  static let bitmap = "image/bmp"
  static let css = "text/css"
  static let gif = "image/gif"
  static let javaScript = "text/javascript"
  static let jpeg = "image/jpeg"
  static let html = "text/html"
  static let octetStream = "application/octet-stream"
  static let passbook = "application/vnd.apple.pkpass"
  static let passbookBundle = "application/vnd.apple.pkpasses"
  static let pdf = "application/pdf"
  static let plainText = "text/plain"
  static let png = "image/png"
  static let webP = "image/webp"
  static let xHTML = "application/xhtml+xml"

  private static let webViewViewableTypes: [String] = [
    MIMEType.bitmap, MIMEType.gif, MIMEType.jpeg, MIMEType.html, MIMEType.pdf, MIMEType.plainText,
    MIMEType.png, MIMEType.webP, MIMEType.xHTML,
  ]

  static func canShowInWebView(_ mimeType: String) -> Bool {
    return webViewViewableTypes.contains(mimeType.lowercased())
  }

  static func mimeTypeFromFileExtension(_ fileExtension: String) -> String {
    guard let mimeType = UTType(filenameExtension: fileExtension)?.preferredMIMEType else {
      return MIMEType.octetStream
    }
    return mimeType
  }
}

extension String {
  var isKindOfHTML: Bool {
    return [MIMEType.html, MIMEType.xHTML].contains(self)
  }
}

class DownloadHelper: NSObject {
  fileprivate let request: URLRequest
  fileprivate let preflightResponse: URLResponse
  fileprivate let cookieStore: WKHTTPCookieStore

  required init?(
    request: URLRequest?,
    response: URLResponse,
    cookieStore: WKHTTPCookieStore,
    canShowInWebView: Bool
  ) {
    guard let request = request else {
      return nil
    }

    let contentDisposition = (response as? HTTPURLResponse)?.value(
      forHTTPHeaderField: "Content-Disposition"
    )
    let mimeType = response.mimeType ?? MIMEType.octetStream
    let isAttachment =
      contentDisposition?.starts(with: "attachment") ?? (mimeType == MIMEType.octetStream)

    guard isAttachment || !canShowInWebView else {
      return nil
    }

    self.cookieStore = cookieStore
    self.request = request
    self.preflightResponse = response
  }

  func downloadAlert(
    from view: UIView,
    okAction: @escaping (HTTPDownload) -> Void
  ) -> UIAlertController? {
    guard let host = request.url?.host, let filename = request.url?.lastPathComponent else {
      return nil
    }

    let download = HTTPDownload(
      cookieStore: cookieStore,
      preflightResponse: preflightResponse,
      request: request
    )

    let expectedSize =
      download.totalBytesExpected != nil
      ? ByteCountFormatter.string(fromByteCount: download.totalBytesExpected!, countStyle: .file)
      : nil

    let title = "\(filename) - \(host)"

    let downloadAlert = UIAlertController(title: title, message: nil, preferredStyle: .actionSheet)

    var downloadActionText = Strings.download
    // The download can be of undetermined size, adding expected size only if it's available.
    if let expectedSize = expectedSize {
      downloadActionText += " (\(expectedSize))"
    }

    let okAction = UIAlertAction(title: downloadActionText, style: .default) { _ in
      okAction(download)
    }

    let cancelAction = UIAlertAction(title: Strings.cancelButtonTitle, style: .cancel)

    downloadAlert.addAction(okAction)
    downloadAlert.addAction(cancelAction)

    downloadAlert.popoverPresentationController?.do {
      $0.sourceView = view
      $0.sourceRect = CGRect(x: view.bounds.midX, y: view.bounds.maxY - 16, width: 0, height: 0)
      $0.permittedArrowDirections = []
    }

    return downloadAlert
  }
}

class OpenPassBookHelper: NSObject {
  private let mimeType: String
  fileprivate var url: URL

  fileprivate let browserViewController: BrowserViewController

  required init?(
    request: URLRequest?,
    response: URLResponse,
    canShowInWebView: Bool,
    forceDownload: Bool,
    browserViewController: BrowserViewController
  ) {
    guard let mimeType = response.mimeType,
      [MIMEType.passbook, MIMEType.passbookBundle].contains(mimeType.lowercased()),
      PKAddPassesViewController.canAddPasses(),
      let responseURL = response.url, !forceDownload
    else { return nil }
    self.mimeType = mimeType
    self.url = responseURL
    self.browserViewController = browserViewController
    super.init()
  }

  @MainActor func open() async {
    do {
      let passes = try await parsePasses()
      if let addController = PKAddPassesViewController(passes: passes) {
        browserViewController.present(addController, animated: true, completion: nil)
      }
    } catch {
      // display an error
      let alertController = UIAlertController(
        title: Strings.unableToAddPassErrorTitle,
        message: Strings.unableToAddPassErrorMessage,
        preferredStyle: .alert
      )
      alertController.addAction(
        UIAlertAction(title: Strings.unableToAddPassErrorDismiss, style: .cancel) { (action) in
          // Do nothing.
        }
      )
      browserViewController.present(alertController, animated: true, completion: nil)
      return
    }
  }

  private func parsePasses() async throws -> [PKPass] {
    if mimeType == MIMEType.passbookBundle {
      let url = try await ZipImporter.unzip(path: url)
      do {
        let files = await enumerateFiles(in: url, withExtensions: ["pkpass", "pkpasses"])
        let result = try files.map { try PKPass(data: Data(contentsOf: $0)) }
        try? await AsyncFileManager.default.removeItem(at: url)
        return result
      } catch {
        try? await AsyncFileManager.default.removeItem(at: url)
        throw error
      }
    }

    let passData = try Data(contentsOf: url)
    return try [PKPass(data: passData)]
  }

  func enumerateFiles(
    in directory: URL,
    withExtensions extensions: [String] = []
  ) async -> [URL] {
    let enumerator = AsyncFileManager.default.enumerator(
      at: directory,
      includingPropertiesForKeys: [.isRegularFileKey]
    )

    var result: [URL] = []
    for await fileURL in enumerator {
      do {
        let resourceValues = try fileURL.resourceValues(forKeys: [.isRegularFileKey])
        if resourceValues.isRegularFile == true, extensions.contains(fileURL.pathExtension) {
          result.append(fileURL)
        }
      } catch {
        Logger.module.error("Error reading file \(fileURL): \(error)")
      }
    }

    return result
  }
}
