Page({
  data: {
    apiKey: '',
    deviceId: ''
  },

  onApiKeyInput(e) {
    this.setData({
      apiKey: e.detail.value
    });
  },

  onDeviceIdInput(e) {
    this.setData({
      deviceId: e.detail.value
    });
  },

  saveSettings() {
    // 保存设置逻辑，实际开发中可以保存到本地或提交到服务器
    wx.setStorageSync('apiKey', this.data.apiKey);
    wx.setStorageSync('deviceId', this.data.deviceId);
    wx.showToast({
      title: '设置已保存',
      icon: 'success'
    });
  },

  // 返回首页
  goToHome() {
    wx.navigateBack();  // 返回上一页，即首页
  }
});
